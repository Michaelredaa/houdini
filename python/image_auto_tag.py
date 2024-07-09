import os
import cv2
import numpy as np
import pandas as pd

from tensorflow.keras.applications import VGG16
from tensorflow.keras.applications.vgg16 import preprocess_input

from sklearn.preprocessing import MultiLabelBinarizer
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.multioutput import MultiOutputClassifier
from sklearn.metrics import accuracy_score, f1_score

import joblib

IMG_SIZE = (224, 224)
data_root = r"C:\Users\michael\Documents\work\houdini\_data"

model = VGG16(weights='imagenet', include_top=False, input_shape=(224, 224, 3))


def extract_features(image_path):
    img = cv2.imread(image_path)
    if img.size == 0:
        return
    img = cv2.resize(img, IMG_SIZE)
    img = img / 255

    img = preprocess_input(img)
    img = np.expand_dims(img, axis=0)
    features = model.predict(img)
    features = features.flatten()
    return features


def modify_preview_image(row):
    image_path = row['PreviewImage']
    image_path = os.path.join(data_root, "images", os.path.basename(image_path))
    if not os.path.isfile(image_path):
        return
    row['PreviewImage'] = image_path
    return row


df = pd.read_csv(os.path.join(data_root, "assets_data.csv"))


# Filter 3d assets only
df = df[df['Category'].str.contains("3d")]

df = df.head(1000)

# prepare data, and make preview images absolute path
df = df.apply(modify_preview_image, axis=1).dropna()
df['Tags'] = df['Tags'].apply(lambda x: str(x).split(', '))
df['Category'] = df['Category'].apply(lambda x: str(x).split(', '))

# Extract features from images based on imagenet model
df['features'] = df['PreviewImage'].apply(lambda x: extract_features(x)).dropna()

# Initialize MultiLabelBinarizer
mlb_tags = MultiLabelBinarizer()
mlb_categories = MultiLabelBinarizer()

# Fit and transform tags and categories
tags_encoded = mlb_tags.fit_transform(df['Tags'])
categories_encoded = mlb_categories.fit_transform(df['Category'])

# Model Training

# Combine image features and text features
X = np.array(list(df['features']))
y = np.hstack([categories_encoded, tags_encoded])

# Train-test split
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Initialize and train the model
rf = RandomForestClassifier(n_estimators=100)
multi_target_rf = MultiOutputClassifier(rf, n_jobs=-1)
multi_target_rf.fit(X_train, y_train)

# Model Evaluation
model_data = {
    'model': multi_target_rf,
    'mlb_categories': mlb_categories,
    'mlb_tags': mlb_tags,
    'X_train': X_train,
    'y_train': y_train,
    'X_test': X_test,
    'y_test': y_test
}


# Save the model
joblib.dump(model_data, os.path.join(data_root, "data_model.pkl"))

# # Calculate accuracy and F1 score for each output
# # Predict on the test set
# y_pred = multi_target_rf.predict(X_test)
#
# for i in range(y_test.shape[1]):
#     print(f'Accuracy for output {i}: {accuracy_score(y_test[:, i], y_pred[:, i])}')
#     print(f'F1 score for output {i}: {f1_score(y_test[:, i], y_pred[:, i], average="weighted")}')
