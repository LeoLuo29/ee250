# Lab 5

## Team Members
Leo Luo

## Lab Question Answers

### Part 1

Answer for Question 1:
dBm stands for decibel-milliwatts. -30 to -70 is considered as a good value

Answer for Question 2:
Because python has no library to check the wifi signal. It has to rely on the native OS
to read wifi signal. the the different commands are for different OS running on the native metal.
For mac: wdutil only prints the RSSI when run as root, so macOS will ask for
your login password the first time (and again after a few minutes).

Answer for Question 3:
it returns a return code to report any issues and errors appeared.

Answer for Question 4:
it checks and search a  regular expression pattern that matches the one provided in the input.

Answer for Question 5:
because windows system does not return the raw dBm data, instead, it returns a quality percentage between 0-100.

Answer for Question 6:
standard deviation tells you how apart / how scattered your data is
my standard deviation is practically and theoratically zero, because i only have a dorm room ;(

Answer for Question 7:
Data frame is the fundamental unit of data link layer.
using data frame because computers need a structured way to track information of different physical and signals.
it is also the payload of the layers above data link layer.
it helps computer to decipher which layer this portion of information belongs to.

Answer for Question 8:
error bar is potted 1 standard deviation above and below the average vallue
it tells how much the measured value varies.

Answer for Question 9:
sorry, i onnly have 1 room. but it does gets tiny bit stronger when i am closer to one particular wall.
I guess the wifi device is in the neighbouring room through that particular wall.

I know it looks the same. Because I dont have a house, i live in a dorm room.

#### Raw samples (dBm)

```
bedroom:     -47 -46 -46 -46 -46 -46 -46 -45 -46 -46
living room: -48 -47 -46 -46 -46 -46 -46 -46 -46 -47
kitchen:     -46 -47 -47 -47 -47 -47 -47 -47 -47 -47
bathroom:    -46 -46 -46 -46 -46 -47 -47 -47 -47 -48
garage:      -47 -47 -47 -47 -47 -47 -48 -47 -47 -48
```
I don't have different rooms, its just my dorm room.

### Part 2

Answer for Question 10:
when moving boards away form access point, signal gets weaker. 
TCP drops more smoothly than UDP because it reacts to loss by slowing sending rates.
UDP keeps the sending rate the same so it drops less smoothly.


Answer for Question 11:
around -70 to -75 dBm

Answer for Question 12:
Because TCP has data managing that gurantees the data will arrive.


Answer for Question 13:
when clinets send far more than UDP can carry, more data that cannot be carried are added, more losses will be there. thus no more useful throughput.

Answer for Question 14:
I think yes. 5 GHz has wider channels and less interferences. 

## Acknowledgments
helped with some codings, and reference checking