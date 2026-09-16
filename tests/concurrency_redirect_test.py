import requests
from concurrent.futures import ThreadPoolExecutor

CODE = "Vmj53u"
URL = f"http://localhost:18080/{CODE}"

def click():
    response = requests.get(
        URL,
        allow_redirects=False
    )
    return response.status_code


with ThreadPoolExecutor(max_workers=20) as executor:
    results = list(executor.map(lambda _: click(), range(100)))


print("Total requests :", len(results))
print("Successful     :", sum(r == 302 for r in results))
print("Failed         :", sum(r != 302 for r in results))