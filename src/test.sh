coverage run --omit '*dist-packages*','reels/__init__.py','reels/Intake.py' -m pytest test_all.py
coverage report -m
