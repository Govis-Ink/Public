import base64

with open('Example.webp', 'rb') as f:
    img_data = base64.b64encode(f.read()).decode('utf-8')

with open('Example.txt', 'w') as f:
    f.write(img_data)