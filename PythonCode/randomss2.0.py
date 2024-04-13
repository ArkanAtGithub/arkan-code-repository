import random
import json
import time

def generate_random_numbers(seed, num_digits, count):
    random.seed(seed)
    random_numbers = []

    print("Generating random numbers...")

    for i in range(count):
        min_value = 0
        max_value = (10 ** num_digits) - 1
        v = random.randint(min_value, max_value)
        random_numbers.append(str(v).zfill(num_digits))  # Adjust to ensure leading zeros
        percentage = (i + 1) / count * 100
        print(f"Progress: {percentage:.2f}% complete", end='\r')

    print("\nRandom numbers generated successfully.")
    return random_numbers

def save_to_json(random_numbers, filename):
    filenamejson = filename + ".json"
    json_output = json.dumps(random_numbers)
    with open(filenamejson, 'w') as f:
        f.write(json_output)
    print("JSON output saved to", filenamejson)

def main():
    print("Randomss.py by Arkan")
    print("For random seed set seed to 0")

    try:
        seed = int(input("Seed: "))
        num_digits = int(input("Number of digits: "))
        count = int(input("How many: "))
        save_output = input("Save output (y/n): ").lower() == 'y'
        filename = input("Filename: ") if save_output else None
    except ValueError:
        print("Error: Invalid input")
        return

    if seed == 0:
        seed = random.randrange(0, 100000000)
        print("Random seed:", seed)

    start_time = time.time()
    random_numbers = generate_random_numbers(seed, num_digits, count)
    end_time = time.time()

    time_taken = end_time - start_time
    print(f"Time taken: {time_taken:.2f} seconds")

    if save_output:
        save_to_json(random_numbers, filename)

    input("Thank you...")

if __name__ == "__main__":
    main()