import requests
import json
import time
import os
import math

# Define the absolute path for the output file
output_file_path = r"Documents/wxnow.txt"

# Define the thingspeak URL with average, round, and results parameters
url = "https://thingspeak.com/channels/2499370/feed.json?average=10&round=1&results=1"

def write_data_to_file(data):
  """
  Writes the extracted data from the ThingSpeak API to a specified file for APRS WX station use.

  Args:
      data (dict): The JSON data retrieved from the ThingSpeak API.
  """
  def clamp(n, minn, maxn):
    return max(min(maxn, n), minn)

  strf = time.strftime("%b %d %Y %H:%M")
  with open(output_file_path, "w") as file:
        # Date and time
        file.write(f"{strf}\n")
        # Filler for wind data
        file.write(f".../...g...")
        # Extract desired information from the JSON data
        # Assuming the data structure includes a "feeds" list with elements containing
        # fields like "field1", "field2", etc.
        for feed in data["feeds"]:
            celsius = float(feed['field1'])
            f = (celsius * 9 / 5) + 32
            file.write(f"t0{math.trunc(f)}")
            rh = feed['field2']
            rh = clamp(float(rh), 0 ,99)
            file.write(f"h{math.trunc(float(rh))}")
            baro = feed['field3']
            baro = clamp(float(baro), 1000, 1010)
            file.write(f"b{int(str(baro).replace('.', ''))}")
            # Add additional lines for other fields as needed

def main():
    """
    Main function that fetches data from ThingSpeak API every 5 minutes and writes it to a file.
    """

    while True:
        # Send a GET request to the URL
        response = requests.get(url)

        # Check for successful response
        if response.status_code == 200:
            # Parse the JSON data
            data = json.loads(response.text)

            # Write the data to the file
            write_data_to_file(data)

            print("Data successfully written to wxnow.txt")
        else:
            print(f"Error: {response.status_code}")

        # Wait for 5 minutes before the next request
        time.sleep(60 * 5)  # (60 * 5 = 300s) 300s = 5m

if __name__ == "__main__":
    main()
