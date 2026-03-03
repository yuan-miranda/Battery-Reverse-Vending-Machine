import cv2
import time
import requests
import numpy as np
from ultralytics import YOLO

model = YOLO("yolo26l.pt")

url = "http://192.168.4.1/capture"

while True:
    res = requests.get(url, timeout=5)
    img = np.array(bytearray(res.content), dtype=np.uint8)
    img = cv2.imdecode(img, cv2.IMREAD_COLOR)

    if img is None:
        continue

    result = model(img, classes=[39])[0]
    if result.boxes is not None:
        annotated_img = result.plot()
        cv2.imshow("YOLOv26l Detection", annotated_img)

    # press 'q' to quit
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

    time.sleep(5)

cv2.destroyAllWindows()
