import requests
from concurrent.futures import ThreadPoolExecutor

URL = "http://localhost:18080/api/v1/urls"

def create_url(i):
    response = requests.post(
        URL,
        json={"url": f"https://example.com/page/{i}"}
    )

    return response.status_code, response.json()


with ThreadPoolExecutor(max_workers=20) as executor:
    results = list(executor.map(create_url, range(100)))


success = [r for r in results if r[0] == 200 or r[0] == 201]

print("Total requests :", len(results))
print("Successful     :", len(success))
print("Failed         :", len(results) - len(success))

codes = [r[1]["code"] for r in success]

print("Unique codes   :", len(set(codes)))
print("Duplicate codes:", len(codes) - len(set(codes)))