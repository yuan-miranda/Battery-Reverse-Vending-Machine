import cv2
from ultralytics import YOLO

# load your trained model
model = YOLO("my_model.pt")

cap = cv2.VideoCapture(0)

while True:
    ret, frame = cap.read()
    if not ret:
        break

    results = model(frame)[0]

    annotated = results.plot()  # draw boxes

    cv2.imshow("Detection", annotated)

    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

cap.release()
cv2.destroyAllWindows()
