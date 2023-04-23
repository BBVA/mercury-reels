# Mercury-Reels

#     Copyright 2023 Banco Bilbao Vizcaya Argentaria, S.A.

#     This product includes software developed at

#     BBVA (https://www.bbva.com/)

#       Licensed under the Apache License, Version 2.0 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

#       Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.

import os

import pandas as pd


PYSPARK = os.environ.get("SPARK_HOME") is not None


if PYSPARK:
    import pyspark
    from pyspark.accumulators import AccumulatorParam

    try:
        SPARK = pyspark.sql.SparkSession.builder.getOrCreate()

    except:             # noqa: E722
        SPARK = None    # PYSPARK is true, but SPARK session is unavailable (happens in workers)

else:
    class AccumulatorParam:
        """Dummy to avoid parsing error when there is no pyspark. In that case it will never be called."""

        def __init__(self):
            pass

    SPARK = None        # A dummy to import when pyspark is not available


class SparkEventsAcc(AccumulatorParam):

    """ This is an internal AccumulatorParam descendant to propagate the tuples created by a user defined function (inside the Intake)
        applied via .foreach() in the workers. It is a reducer in a map/reduce paradigm generating a complete reduced result in the driver.

        This specific one is able to manage a reels.Events object and feed the tuples to it via its .insert_row() method.

        CAVEAT!!: This object collects all the tuples in lists before they are applied to a unique reels.Events object in the driver.
        It is intended to operate in environments where data is not too big (compared to available RAM in the driver) and computation
        resources (number of workers) is high and therefore the process will benefit from parallelism.
        In any other setting, you should not use this class. This class is created when you construct an Intake object with
        spark_method == 'accumulator'. The much safer (and possibly slower) spark_method == 'local_iterator' will not require a lot of RAM.
    """

    def zero(self, value):
        return []

    def addInPlace(self, acc, aux):

        if type(acc) == list:
            if type(aux) == list:
                acc.extend(aux)
            else:
                acc.append(aux)

        else:
            assert type(aux) == list

            for tup in aux:
                e, d, w = tup
                acc.insert_row(e, d, w)

        return acc


if PYSPARK and SPARK is not None:
    events_acc = SPARK.sparkContext.accumulator(None, SparkEventsAcc())     # noqa: F821


class SparkClipsAcc(AccumulatorParam):

    """ This is an internal AccumulatorParam descendant to propagate the tuples created by a user defined function (inside the Intake)
        applied via .foreach() in the workers. It is a reducer in a map/reduce paradigm generating a complete reduced result in the driver.

        This specific one is able to manage a reels.Clips object and feed the tuples to it via its .scan_event() method.

        CAVEAT!!: This object collects all the tuples in lists before they are applied to a unique reels.Clips object in the driver.
        It is intended to operate in environments where data is not too big (compared to available RAM in the driver) and computation
        resources (number of workers) is high and therefore the process will benefit from parallelism.
        In any other setting, you should not use this class. This class is created when you construct an Intake object with
        spark_method == 'accumulator'. The much safer (and possibly slower) spark_method == 'local_iterator' will not require a lot of RAM.
    """

    def zero(self, value):
        return []

    def addInPlace(self, acc, aux):

        if type(acc) == list:
            if type(aux) == list:
                acc.extend(aux)
            else:
                acc.append(aux)

        else:
            assert type(aux) == list

            for tup in aux:
                e, d, w, c, t = tup
                acc.scan_event(e, d, w, c, t)

        return acc


if PYSPARK and SPARK is not None:
    clips_acc = SPARK.sparkContext.accumulator(None, SparkClipsAcc())   # noqa: F821


class Intake:

    """Utility class to efficiently populate any reels object with data either from pandas or pyspark dataframes.

    This object implements data populating methods (in plural) that call the equivalent methods (in singular) over a complete dataframe.

      - insert_rows()    is Events.insert_row() for each row
      - define_events()  is Events.define_event() for each row
      - scan_events()    is Clips.scan_event() for each row
      - insert_targets() is Targets.insert_target() for each row

    Args:
        dataframe:    Either a pandas or a pyspark dataframe with the data to be loaded into reels objects.
        spark_method: This only applies to pyspark dataframe. It has two possible values 'local_iterator' (default) the safest and less
                      RAM consuming. If your environment has many workers (and therefore you would want to improve performance via
                      parallelism) and you have enough RAM in the driver to hold a list of tuples with the values you want to load,
                      you can try the more efficient but also more experimental 'accumulator' value.
    """

    def __init__(self, dataframe, spark_method='local_iterator'):
        self.pd_data = None
        self.sp_data = None

        if type(dataframe) == pd.DataFrame:
            self.pd_data = dataframe

            return

        if PYSPARK and type(dataframe) == pyspark.sql.dataframe.DataFrame:
            self.sp_data = dataframe

            self.use_accumulator = (spark_method == 'accumulator') and SPARK is not None

            return

        raise ValueError

    def __str__(self):
        if self.pd_data is not None:
            rows, cols = self.pd_data.shape
            names = ', '.join(list(self.pd_data.columns))
            title = 'reels.Intake object using Pandas'
        else:
            rows = self.sp_data.count()
            cols = len(self.sp_data.columns)
            names = ', '.join(self.sp_data.columns)
            title = 'reels.Intake object using pyspark %s' % ['(iterator)', '(accumulator)'][int(self.use_accumulator)]

        return '%s\n\nColumn names: %s\n\nShape: %i x %i' % (title, names, rows, cols)

    def __repr__(self):
        return self.__str__()

    def insert_rows(self, events, columns=None):
        """Populate an Events object calling events.insert_row() over the entire dataframe.

        Args:
            events:  The Events object to be filled with data.
            columns: A list with the names of the three columns containing (emitter, description, weight) in the dataframe.
                     The default value is ['emitter', 'description', 'weight'].
        """

        if columns is None:
            columns = ['emitter', 'description', 'weight']

        if self.sp_data is not None and self.use_accumulator:
            events_acc.value = events

            def f(row):
                global events_acc

                tup = (
                    str(row[columns[0]]),
                    str(row[columns[1]]),
                    float(row[columns[2]]),
                )

                events_acc += tup

            self.sp_data.foreach(f)

            return

        lambda_f = lambda row: events.insert_row(                               # noqa: E731
            str(row[columns[0]]), str(row[columns[1]]), float(row[columns[2]])
        )

        if self.pd_data is not None:
            self.pd_data.apply(lambda_f, axis=1)
        else:
            for row in self.sp_data.rdd.toLocalIterator():
                lambda_f(row)

    def define_events(self, events, columns=None):
        """Populate an Events object calling events.define_event() over the entire dataframe.

        Args:
            events:  The Events object to be filled with data.
            columns: A list with the names of the four columns containing (emitter, description, weight, code) in the dataframe.
                     The default value is ['emitter', 'description', 'weight', 'code'].
        """

        if columns is None:
            columns = ['emitter', 'description', 'weight', 'code']

        lambda_f = lambda row: events.define_event(     # noqa: E731
            str(row[columns[0]]),
            str(row[columns[1]]),
            float(row[columns[2]]),
            int(row[columns[3]]),
        )

        if self.pd_data is not None:
            self.pd_data.apply(lambda_f, axis=1)
        else:
            for row in self.sp_data.rdd.toLocalIterator():
                lambda_f(row)

    def scan_events(self, clips, columns=None):
        """Populate a Clips object calling clips.scan_event() over the entire dataframe.

        Args:
            clips:   The Clips object to be filled with data.
            columns: A list with the names of the five columns containing (emitter, description, weight, client, time) in the dataframe.
                     The default value is ['emitter', 'description', 'weight', 'client', 'time'].
        """

        if columns is None:
            columns = ['emitter', 'description', 'weight', 'client', 'time']

        if self.sp_data is not None and self.use_accumulator:
            clips_acc.value = clips

            def f(row):
                global clips_acc

                tup = (
                    str(row[columns[0]]),
                    str(row[columns[1]]),
                    float(row[columns[2]]),
                    str(row[columns[3]]),
                    str(row[columns[4]]),
                )

                clips_acc += tup

            self.sp_data.foreach(f)

            return

        lambda_f = lambda row: clips.scan_event(    # noqa: E731
            str(row[columns[0]]),
            str(row[columns[1]]),
            float(row[columns[2]]),
            str(row[columns[3]]),
            str(row[columns[4]]),
        )

        if self.pd_data is not None:
            self.pd_data.apply(lambda_f, axis=1)
        else:
            for row in self.sp_data.rdd.toLocalIterator():
                lambda_f(row)

    def insert_targets(self, targets, columns=None):
        """Populate a Targets object calling targets.insert_target() over the entire dataframe.

        Args:
            targets: The Targets object to be filled with data.
            columns: A list with the names of the two columns containing (client, time) in the dataframe.
                     The default value is ['client', 'time'].
        """

        if columns is None:
            columns = ['client', 'time']

        lambda_f = lambda row: targets.insert_target(   # noqa: E731
            str(row[columns[0]]), str(row[columns[1]])
        )

        if self.pd_data is not None:
            self.pd_data.apply(lambda_f, axis=1)
        else:
            for row in self.sp_data.rdd.toLocalIterator():
                lambda_f(row)
