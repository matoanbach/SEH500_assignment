- Create and activate a python virtual environment
  **- Note:** If there is any virtual existed before, delete it and create a new one

```sh
python3 -m venv venv # create a virtual environment named "venv" at the current directory
source venv/bin/activate # activate the virtual environment
```

- Verify if the virtual environment is activated

```sh
which python3 # expected: /Users/tieuma/Library/CloudStorage/OneDrive-Seneca/Seneca/SEMESTER5/SED500/a4/a4/venv/bin/python3
which pip3 # expected: /Users/tieuma/Library/CloudStorage/OneDrive-Seneca/Seneca/SEMESTER5/SED500/a4/a4/venv/bin/pip3
```

- Install dependencies

```sh
pip3 install -r requirements.txt
```

- Verify K66F connected with your computer

```sh
ls /dev/tty.*
```

- Find the com port K66F is sending data to

```sh
screen /dev/tty.usbmodemXXXXXX [baudrate] #replace /dev/tty.usbmodemXXXXXX with the port K66F is connected to and baudrate is 115200 as default
```

**Note:** after verifying, remember to close it so that the python script can use the port

- Run the visualization

```sh
python3 plot.py
```

**Note:** Click "z" to change to yaw mode and click "esc" to quit the program

- Clean your enviroment

```sh
rm -rf venv
```