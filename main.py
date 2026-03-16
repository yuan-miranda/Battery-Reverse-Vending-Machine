import os
import time
from flask import Flask
import cv2
import requests
from ultralytics import YOLO

app = Flask(__name__)
model = YOLO("my_model.pt")

# http://192.168.4.1/capture
stream_url = "http://192.168.4.1:81/stream"
control_url = "http://192.168.4.1/control"

os.makedirs("img", exist_ok=True)


def send_command(command):
    try:
        requests.get(control_url, params={"var": command, "val": 1}, timeout=5)
    except:
        pass


@app.route("/scan")
def scan():
    cap = cv2.VideoCapture(stream_url)
    time.sleep(0.5)

    for _ in range(10):
        cap.grab()

    ret, frame = cap.read()
    if not ret:
        return "Failed to capture image", 500

    result = model(frame)[0]
    annotated_frame = result.plot()

    cv2.imwrite(f"img/annotated.jpg", annotated_frame)

    if len(result.boxes) > 0:
        cls_id = int(result.boxes[0].cls[0])
        label = model.names[cls_id]

        if label == "1.5v":
            send_command("accept_1.5v")
        elif label == "9v":
            send_command("accept_9v")
        return f"Battery detected: {label}"
    else:
        send_command("reject")
        return "No battery detected"


# while True:
#     ret, new_frame = cap.read()
#     if ret:
#         frame = new_frame

#     if ser.in_waiting > 0:
#         line = ser.readline().decode().strip()

#         if line == 'BRVM_SCAN' and frame is not None:
#             result = model(frame)[0]

#             if len(result.boxes) > 0:
#                 cls_id = int(result.boxes[0].cls[0])
#                 label = model.names[cls_id]

#                 cv2.imshow('Battery RVM', result.plot())

#                 if label == '1.5v':
#                     send_command('accept_1.5v')
#                 elif label == '9v':
#                     send_command('accept_9v')
#             else:
#                 cv2.imshow('Battery RVM', frame)
#                 send_command('reject')

#     if cv2.waitKey(1) & 0xFF == ord('q'):
#         break


# ser.close()
# cv2.destroyAllWindows()

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=False)

