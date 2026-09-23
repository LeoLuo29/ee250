import requests

BASE_URL = "https://api.weatherapi.com/v1/current.json"
WEATHER_API_KEY = "YOUR_WEATHERAPI_KEY"


def get_weather(city):
    # TODO 1:
    # Create the query parameters:
    # key=<WEATHER_API_KEY>, q=<city>, aqi=no
    params = {}

    # TODO 2:
    # Send the GET request with timeout=10.
    response = None

    # TODO 3:
    # Print the HTTP status code.
    # If the status is not 200, print the returned error and stop.

    # TODO 4:
    # Parse response.json() and print:
    # - location name
    # - region and country
    # - temperature in Fahrenheit
    # - condition text
    # - humidity


if __name__ == "__main__":
    city = input("City: ").strip()
    get_weather(city)
