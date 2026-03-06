import cv2
import time
import requests
from ultralytics import YOLO

model = YOLO("yolo26l.pt")

# http://192.168.4.1/capture
stream_url = "http://192.168.4.1:81/stream"
control_url = "http://192.168.4.1/control"

cap = cv2.VideoCapture(stream_url)
last_capture = 0


def send_command(command):
    try:
        requests.get(control_url, params={"var": command, "val": 1}, timeout=5)
    except Exception as e:
        print(f"Error sending command '{command}': {e}")


while True:
    ret, frame = cap.read()
    if not ret:
        continue

    if time.time() - last_capture > 5:
        result = model(frame, classes=[39])[0]

        if len(result.boxes) > 0:
            annotated_img = result.plot()
            cv2.imshow("Battery RVM", annotated_img)
            send_command("accept")
        else:
            cv2.imshow("Battery RVM", frame)
            send_command("reject")

        last_capture = time.time()

    # press 'q' to quit
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

cv2.destroyAllWindows()
