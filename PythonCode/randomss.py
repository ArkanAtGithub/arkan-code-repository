import random
import json
import time
import sys

#bypass integer to string conversion limit
sys.set_int_max_str_digits(0)

x = 0
s = 0
print("Randomss.py by Arkan")
print("For random seed set seed to 0")
try:
    s = int(input("Seed: "))
    num_digits = int(input("Number of digits: "))
    x = int(input("How many : "))
    name = input("Filename: ")
except ValueError:
    print("Error. Invalid input")
    exit()

if s == 0:
    s = random.randrange(0, 100000000)
    sr = str(s)
    print("Random seed: " + sr)

random.seed(s)
random_numbers = []

print("Warning: JSON output will not be saved if user exit the program")

print("Generating random numbers...")
start_time = time.time()

for i in range(x):
    min_value = 10 ** (num_digits - 1)
    max_value = (10 ** num_digits) - 1
    v = random.randint(min_value, max_value)
    random_numbers.append(v)
    
    # Calculate percentage completion
    percentage = (i + 1) / x * 100
    print(f"Progress: {percentage:.2f}% complete", end='\r')
end_time = time.time()
time_taken = end_time - start_time
print(f"\nTime taken: {time_taken:.2f} seconds")

json_output = json.dumps(random_numbers)
filename = name + ".json"

# Save JSON output to a file
with open(filename, 'w') as f:
    f.write(json_output)

print("JSON output saved to", filename)

input("Thank you...")