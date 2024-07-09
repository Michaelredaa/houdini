# -*- coding: utf-8 -*-
"""
Documentation:
"""
import os
from image_auto_tag import extract_features
import joblib

from tensorflow.keras.applications import VGG16
from tensorflow.keras.applications.vgg16 import preprocess_input

data_root = r"C:\Users\michael\Documents\work\houdini\_data"


def predict_image(image_path, model, mlb_categories, mlb_tags):
    features = extract_features(image_path)
    features = features.reshape(1, -1)  # Reshape to match model input

    # Predict categories and tags
    predictions = model.predict(features)
    predicted_categories = mlb_categories.inverse_transform(
        predictions[:, :len(mlb_categories.classes_)])
    predicted_tags = mlb_tags.inverse_transform(predictions[:, len(mlb_categories.classes_):])

    return predicted_categories, predicted_tags


new_image_path = r'C:\Users\michael\Downloads\uc1kebzfa_Thumb_HighPoly_Retina.png'


model_data = joblib.load(os.path.join(data_root, "data_model.pkl"))

multi_target_rf = model_data['model']
mlb_categories = model_data['mlb_categories']
mlb_tags = model_data['mlb_tags']

predicted_categories, predicted_tags = predict_image(new_image_path, multi_target_rf,
                                                     mlb_categories, mlb_tags)

print(f'Predicted Categories: {predicted_categories}')
print(f'Predicted Tags: {predicted_tags}')

if __name__ == '__main__':
    print(__name__)
