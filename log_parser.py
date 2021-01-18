import matplotlib.pyplot as plt
import argparse

parser = argparse.ArgumentParser(description="ES1 CanSat logfile parser")
parser.add_argument("-f", "--filename", default="log.txt")
parser.add_argument("-t", "--temperature", action="store_true", help="Show temperature data on graph")
parser.add_argument("-p", "--pressure", action="store_true", help="Show pressure data on graph")
args = parser.parse_args()

with open(args.filename, "r") as fp:
	data = fp.read().strip().split("\n")

plt.rcParams.update({'font.size': 14})

timeline = []
temperatures = []
pressures = []

fig,ax = plt.subplots(1)

for line in data:
	line = line.split(" ")

	#Read all the data on one line
	gpsdate = line[0]
	gpstime = line[1]
	latitude = line[3]
	longitude = line[5]
	altitude = line[6]
	speed = line[7]
	temperature = line[8]
	pressure = line[9]
	pitch = line[10]
	roll = line[12]

	#GPS time format
	gpstime = gpstime.split(":")
	for i, x in enumerate(gpstime):
		if len(x) != 2:
			gpstime[i] = "0" + gpstime[i]
	gpstime = ":".join(gpstime)

	timeline.append(gpstime)
	temperatures.append(float(temperature[0:-2]))
	pressures.append(float(pressure[0:-3]))


if args.temperature:
	ax.plot(timeline, temperatures, label = "Temperatuur")
	ax.set_ylabel('Temperatuur *C')

if args.pressure:
	ax.plot(timeline, pressures, label = "Õhurõhk")
	ax.set_ylabel('Õhurõhk kPa')

if args.temperature and args.pressure:
	ax.set_ylabel('Temperatuur ja õhurõhk *C/kPa')

ax.set_xlabel('Kellaaeg')
plt.title('ES1 logifail')
ax.legend()
labels = ax.get_xticklabels()
plt.grid(True)
ax.tick_params(axis='x', labelsize=12)
ticks = ax.get_xticks()[0::36] # Skip some x-labels, too crammed otherwise
ax.set_xticks(ticks)
plt.show()
