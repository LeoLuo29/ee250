import platform
import time
import pandas as pd
import plotly.express as px
import numpy as np
import subprocess
import re

def get_wifi_signal_strength() -> int:
    """Get the signal strength of the wifi connection.
    
    Returns:
        The signal strength in dBm.
    """
    # Question 1: What is dBm? What values are considered good and bad for WiFi signal strength?
        # dBm stands for decibel-milliwatts. -60 to -70 is considered as a good value

    # Question 2: Why do we need to check the OS? What is the difference between the commands for each OS?
        # Because python has no library to check the wifi signal. It has to rely on the native OS
        # to read wifi signal. the the different commands are for different OS running on the native metal.
        # For mac: wdutil only prints the RSSI when run as root, so macOS will ask for 
        # your login password the first time (and again after a few minutes).


    # Question 3: In your own words, what is subprocess.check_output doing? What does it return?
    # HINT: https://docs.python.org/3/library/subprocess.html#subprocess.check_output
    # it returns a return code to report any issues and errors appeared.

    # Question 4: In your own words, what is re.search doing? What does it return?
    # HINT: https://docs.python.org/3/library/re.html#re.search
    # it checks and search a  regular expression pattern that matches the one provided in the input.

    # Question 5: In the Windows case, why do we need to convert the signal quality to dBm?
    # HINT: https://learn.microsoft.com/en-us/windows/win32/api/wlanapi/ns-wlanapi-wlan_association_attributes?redirectedfrom=MSDN
    # because windows system does not return the raw dBm data, instead, it returns a quality percentage between 0-100.

    if platform.system() == 'Linux': # Linux
        output = subprocess.check_output("iwconfig wlan0", shell=True)
        match = re.search(r"Signal level=(-?\d+) dBm", output.decode('utf-8'))
        signal_strength = int(match.group(1))
    elif platform.system() == 'Windows': # Windows
        output = subprocess.check_output("netsh wlan show interfaces", shell=True)
        match = re.search(r"Signal\s*:\s*(\d+)%", output.decode('utf-8'))
        signal_quality = int(match.group(1))
        signal_strength = -100 + signal_quality / 2
    elif platform.system() == 'Darwin': # Mac
        # wdutil only prints the RSSI when run as root, so macOS will ask for
        # your login password the first time (and again after a few minutes).
        output = subprocess.check_output("sudo wdutil info", shell=True, stderr=subprocess.STDOUT)
        # The regex looks for "RSSI" (Received Signal Strength Indicator).
        match = re.search(r"RSSI\s+:\s*(-?\d+)", output.decode('utf-8', errors='ignore'))
        signal_strength = int(match.group(1))
    else:
        raise Exception("Unknown OS")

    print(signal_strength)  # one line per sample, so you can see the readings arrive
    return signal_strength

def main():
    # Choose at least 5 locations to sample the signal strength at
    # These can be rooms in your house, hallways, different floors, outside, etc. (as long as you can get a WiFi signal)
    locations = ['bedroom', 'living room', 'kitchen', 'bathroom', 'garage']
    samples_per_location = 10 # number of samples to take per location
    time_between_samples = 1 # time between samples (in seconds)

    data = [] # list of data points
    for location in locations:
        print(f"Go to the {location} and press enter to start sampling")
        input() # wait for the user to press enter
        signal_strengths = [] # list of signal strengths at this location

        # TODO: collect 10 samples of the signal strength at this location, waiting 1 second between each sample
        # HINT: use the get_wifi_signal_strength function
        for _ in range(samples_per_location):
            signal_strengths.append(get_wifi_signal_strength())
            time.sleep(time_between_samples)

        # TODO: calculate the mean and standard deviation of the signal strengths you collected at this location
        signal_strength_mean = np.mean(signal_strengths)
        signal_strength_std = np.std(signal_strengths)

        # Question 6: What is the standard deviation? Why is it useful to calculate it?
        # standard deviation tells you how apart / how scattered your data is
        # my standard deviation is practically and theoratically zero, because i only have a dorm room ;(


        data.append((location, signal_strength_mean, signal_strength_std))

    # create a dataframe from the data
    df = pd.DataFrame(data, columns=['location', 'signal_strength_mean', 'signal_strength_std'])

    # Question 7: What is a dataframe? Why is it useful to use a dataframe to store the data?
    # HINT: https://pandas.pydata.org/pandas-docs/stable/reference/api/pandas.DataFrame.html
    # HINT: print the dataframe to see what it looks like
    # print(df)
    # Data frame is the fundamental unit of data link layer. 
    # using data frame because computers need a structured way to track information of different physical and signals.
    # it is also the payload of the layers above data link layer. 
    # it helps computer to decipher which layer this portion of information belongs to.

    # save the raw numbers too - handy for your README
    df.to_csv("signal_strength.csv", index=False)

    # TODO: plot the data as a bar chart using plotly - fill in the arguments of px.bar below
    # HINT: https://plotly.com/python/bar-charts/
    # NOTE: use the error_y parameter of px.bar to plot the error bars (1 standard deviation)
    #   documentation: https://plotly.com/python-api-reference/generated/plotly.express.bar.html
    #   (keep ONE px.bar call - a second, empty px.bar() after this one would overwrite your plot)
    fig = px.bar(
        df,
        x='location',
        y='signal_strength_mean',
        error_y='signal_strength_std',
        title='WiFi Signal Strength by Location',
        labels={'location': 'Location', 'signal_strength_mean': 'Mean Signal Strength (dBm)'},
    )
    # Question 8: Why is it important to plot the error bars? What do they tell us?
        # error bar is potted 1 standard deviation above and below the average vallue
        # it tells how much the measured value varies. 

    # write the plot to a file - make sure to commit the PNG file to your repository along with your code
    fig.write_image("signal_strength.png")

    # Question 9: What did you observe from the plot? How does the signal strength change as you move between locations?
    #             Why do you think signal strength is weaker in certain locations?
    # sorry, i onnly have 1 room. but it does gets tiny bit stronger when i am closer to one particular wall. 
    # I guess the wifi device is in the neighbouring room through that particular wall.


if __name__ == "__main__":
    main()
