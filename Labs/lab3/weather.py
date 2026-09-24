import requests

BASE_URL = "https://api.weatherapi.com/v1/current.json"
WEATHER_API_KEY = "3b6587f5268e4f28ae950150262409"


def get_weather(city):
    # TODO 1:
    # Create the query parameters:
    # key=<WEATHER_API_KEY>, q=<city>, aqi=no
    params = {"key": WEATHER_API_KEY, "q": city, "aqi": "no"}

    # TODO 2:
    # Send the GET request with timeout=10.
    response = requests.get(BASE_URL, params=params, timeout=10)

    # TODO 3:
    # Print the HTTP status code.
    # If the status is not 200, print the returned error and stop.
    print("Status:", response.status_code)
    if response.status_code != 200:
        try:
            print("Error:", response.json()["error"]["message"])
        except (ValueError, KeyError):
            print("Error:", response.text)
        return

    # TODO 4:
    # Parse response.json() and print:
    # - location name
    # - region and country
    # - temperature in Fahrenheit
    # - condition text
    # - humidity
    data = response.json()
    location = data["location"]
    current = data["current"]
    print("Location:", location["name"])
    print("Region/Country:", f"{location['region']}, {location['country']}")
    print("Temperature:", f"{current['temp_f']} °F")
    print("Condition:", current["condition"]["text"])
    print("Humidity:", f"{current['humidity']}%")


if __name__ == "__main__":
    city = input("City: ").strip()
    get_weather(city)
