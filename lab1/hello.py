from platform import platform
from datetime import datetime
name = input("Your name: ")
print(f"Hello, {name}!")
print(f"Time: {datetime.now()}")
print(f"System: {platform()}")
