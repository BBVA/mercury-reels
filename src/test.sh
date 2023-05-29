# coverage run --omit '*dist-packages*','reels/__init__.py','reels/Intake.py' -m pytest test_all.py
coverage run --omit '/usr/lib/*','reels/__init__.py','reels/Intake.py' -m pytest test_all.py

coverage report -m
