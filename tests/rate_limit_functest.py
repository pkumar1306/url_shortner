import requests

url = "http://localhost:18080/api/v1/urls"

for i in range(21):
    response = requests.post(
        url,
        json={"url": f"https://example.com/{i}"}
    )

    print(i + 1, response.status_code, response.text)