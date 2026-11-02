import pandas as pd
import numpy as np
import os
import sys
import matplotlib.pyplot as plt
import warnings

import serial
import time

# Configure the serial connection
port = "COM7"
baud_rate = 115200

warnings.filterwarnings("ignore", category=DeprecationWarning)
warnings.filterwarnings("ignore", category=FutureWarning)

parentPath = os.getcwd()

N = np.inf
#xHat = np.array([np.array([1.0]), np.array([0.0]), np.array([0.0])])
xHat = np.array([1.0, 0.0, 0.0])
"""initial covariance"""
sigmaX = np.diag([1e-5, 1e-5, 1e-5])

"""process noise covariance"""
socVar = 1e1
iRC1Var = 1e1
iRC2Var = 1e1
sigmaW = np.diag([socVar, iRC1Var, iRC2Var])
print(sigmaW)

"""sensor noise covariance"""
sigmaV = np.array([np.array([1e-3])])

socStore = np.zeros((490, 1)).flatten()

result_path = r"C:\Users\hp\Downloads\ekf_output.csv"
try:
    storeDF = pd.read_csv(result_path)
except:
    df = pd.DataFrame(columns=["voltage", "current", "soc"])
    df.to_csv(result_path, index= False)
    storeDF = pd.read_csv(result_path)

dt = 1.0
eta = 1.0


def importmodel():

    new_path = r"C:\Users\hp\Downloads\modelling\data-driven-modelling-of-li-ion-batteries\results\OCV--25degC--549_C20DisCh.csv"
    dfOCV = pd.read_csv(new_path)

    global timeOCV, voltOCV, SOCOCV, capacityOCV, r0, r1, r2, c1, c2, RC1, RC2

    timeOCV = dfOCV["time"].to_numpy()
    voltOCV = dfOCV["OCV"].to_numpy()
    SOCOCV = dfOCV["SOC"].to_numpy()
    capacityOCV = dfOCV["disCapacity"].to_numpy()[0]

    new_path_2 = r"C:\Users\hp\Downloads\modelling\data-driven-modelling-of-li-ion-batteries\results\CellParams--25degC--551_Mixed1.csv"

    dfCellParamsOpti = pd.read_csv(new_path_2)
    print(dfCellParamsOpti)

    r0 = dfCellParamsOpti["r0"].to_numpy()
    r1 = dfCellParamsOpti["r1"].to_numpy()
    r2 = dfCellParamsOpti["r2"].to_numpy()
    c1 = dfCellParamsOpti["c1"].to_numpy()
    c2 = dfCellParamsOpti["c2"].to_numpy()
    RC1 = np.exp(-dt / (r1 * c1))
    RC2 = np.exp(-dt / (r2 * c2))
    print("Dt: ", dt)
    print("RC1",RC1)
    print("RC2" ,RC2)

def importdf( filename):


    temp_path = r"\data-driven-modelling-of-li-ion-batteries\datasets\lg-18650hg2\LG_HG2_Original_Dataset_McMasterUniversity_Jan_2020\25degC/"

    temp_path = parentPath + temp_path + filename
    print(temp_path)

    global df

    df = pd.read_csv(
        temp_path, dtype=str
    )
    global volt, curr, disCap, dt, eta

    #df = pd.read_csv(path)
    df = df.apply(pd.to_numeric, errors="ignore")
    print(df.head(10))
    progTime = [
        progTime for progTime in df["Time"]
    ]

    time = [progTime for progTime in progTime]
    df["Time"] = [time for time in time]
    volt = np.asarray([voltage for voltage in df["Voltage"]])
    curr = np.asarray([-current for current in df["Current"]])
    disCap = np.asarray([capacity for capacity in df["Capacity"]])
    dt = np.mean(np.diff(time))
    print("dt: ",dt)
    eta = 1.0


def genInput(i):
    z = np.array([curr[i]])
    z = z + sigmaV @ np.random.randn(1, 1)
    return z.astype(float)

def genMeasurement(i):
    z = np.array([volt[i]])
    z = z + sigmaV @ np.random.randn(1, 1)
    return z.astype(float)

def gendOCVSOC(soc):
    dZ = np.mean(np.diff(SOCOCV))
    print("dz",dZ)

    index = np.argmin(abs(SOCOCV - soc))
    if index != (len(SOCOCV) - 1):
        #if not at the end of array.
        dOCVSOC_fwd = abs(voltOCV[index] - voltOCV[index + 1]) / dZ
    if index != 0:
        #if not at index 0
        dOCVSOC_rvs = abs(voltOCV[index - 1] - voltOCV[index]) / dZ
    if index == (len(SOCOCV) - 1):
        dOCVSOC_fwd = dOCVSOC_rvs
    if index == 0:
        dOCVSOC_rvs = dOCVSOC_fwd
    dOCVSOC = -(dOCVSOC_fwd + dOCVSOC_rvs) / 2

    return dOCVSOC


def iterKF(V, I, k):
    global xHat, sigmaX
    """generate input and measurement"""
    #I = genInput(k)
    #V = genMeasurement(k)
    print("xhat[0]",xHat[0])

    dOCVSOC = gendOCVSOC(xHat[0])
    print(f"V: {V}, I:{I}")
    print(dOCVSOC)
    OCV = voltOCV[np.argmin(abs(SOCOCV - xHat[0]))]
    print(OCV)

    """EKF step 0: compute Ahat, Bhat, Chat, Dhat"""
    Ahat = np.diag([1, RC1.item(0), RC2.item(0)])

    Bhat = np.array(
        [
            np.array([-dt * eta / (3600 * capacityOCV)]),
            1 - RC1,
            1 - RC2,
        ]
    ).T

    Chat = np.array([np.array([dOCVSOC]), -r1, -r2]).T
    #Chat = np.array([[dOCVSOC, -r1[0], -r2[0]]])
    Dhat = np.array([1])

    """EKF step 1: state estimate prediction update"""
    xHat[0] = xHat[0] - (dt * I * eta / (3600 * capacityOCV))

    xHat[1] = xHat[1] * RC1 + (1 - RC1) * I
    xHat[2] = xHat[2] * RC2 + (1 - RC2) * I

    """EKF step 1b: state covariance prediction update"""
    sigmaX = np.dot( np.dot(Ahat, sigmaX), Ahat.T) + np.dot( np.dot(Bhat, sigmaW), Bhat.T)
    print(sigmaX)

    """EKF step 1c: predict system output"""
    yHat = (
        OCV - r0 * I - r1 * xHat[1] - r2 * xHat[2]
    )
    print("yHat: ", yHat)

    """EKF step 2a: compute kalman gain"""
    sigmaY = np.dot(np.dot(Chat, sigmaX), Chat.T) + np.dot( np.dot(Dhat, sigmaV), Dhat.T )
    L = np.dot(np.dot(sigmaX, Chat.T), np.linalg.inv(sigmaY))

    """EKF step 2b: state estimate measurement update"""
    innovation = V - yHat
    xHat = xHat + np.dot(L, innovation)

    """EKF step 2c: state covriance measurement update"""
    sigmaX = sigmaX - np.dot(np.dot(L, sigmaY), L.T)

    """make EKF robust"""
    """limit state values"""

    xHat[0] = np.clip(xHat[0], 0, 1)

    """make sure state covariance is positive definite"""
    D, S, Vsvd = np.linalg.svd(sigmaX)

    HH = Vsvd * S * np.transpose(Vsvd)
    sigmaX = (
        sigmaX + np.transpose(sigmaX) + HH + np.transpose(HH)
    ) / 4

    """get the final terminal voltage after KF update"""
    yHatKF = OCV - r1 * xHat[1] - r2 * xHat[2] - r0 * I

    return xHat[0]

def runKF():
        """import dataframe"""
        importmodel()

        """reserve storage for variables"""
        global socStore
        socStore[0] = xHat[0]

        try:
            ser = serial.Serial(port, baud_rate, timeout=1)
            time.sleep(1)  # Wait for ESP32 to initialize

            print("Connected to ESP32. Reading data...")
            k = 0

            while True:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8').strip()
                    try:
                        voltage, current = map(float, line.split(","))
                        print(f"Voltage: {voltage:.4f} V, Current: {current:.5f} A")

                        soc = iterKF(voltage, current*-1, k)

                        voltage = round(voltage, 2)
                        current = round(current, 4)
                        soc = round(soc, 4) * 100

                        new_row = {"voltage": voltage, "current": current, "soc": soc}
                        storeDF.loc[len(storeDF)] = new_row
                        storeDF.to_csv(result_path, index= False)
                        k = k + 1
                    except ValueError:
                        print("Error parsing data:", line)
                        lis = line.split(",")
                        voltage = float(lis[0])
                        current = float(lis[1])

                        print(f"Voltage: {voltage:.4f} V, Current: {current:.5f} A")

                        soc = iterKF(voltage, current*-1, k)
                        voltage = round(voltage, 2)
                        current = round(current, 4)
                        soc = round(soc, 4) * 100

                        new_row = {"voltage": voltage, "current": current, "soc": soc}
                        storeDF.loc[len(storeDF)] = new_row
                        storeDF.to_csv(result_path, index= False)
                        k = k + 1


        except serial.SerialException as e:
            print(f"Error: {e}")
        except KeyboardInterrupt:
            print("Exiting...")
        finally:
            if 'ser' in locals() and ser.is_open:
                ser.close()
                print("Serial port closed.")

sys.path.append(os.path.dirname(os.path.realpath(os.getcwd())))

filename = "00001.csv"

parentPath = os.path.dirname(os.getcwd())
runKF()


fig2, ax1 = plt.subplots(figsize=(12,6))
storeDF.plot(y=['SOC'], ax=ax1)
storeDF.plot.line(y=['trueSOC'], ax=ax1)
plt.grid()

plt.show()
