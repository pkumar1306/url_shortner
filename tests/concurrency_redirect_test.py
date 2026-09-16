import os
from concurrent.futures import ThreadPoolExecutor

import requests

code = os.environ.get("TEST_CODE")
base_url = os.environ.get("TEST_BASE_URL", "http://localhost:8080").rstrip("/")
if not code:
    raise SystemExit("Set TEST_CODE before running this test.")

URL = f"{base_url}/{code}"

def click():
    response = requests.get(
        URL,
        allow_redirects=False
    )
    return response.status_code


with ThreadPoolExecutor(max_workers=20) as executor:
    results = list(executor.map(lambda _: click(), range(100)))


assert all(result == 302 for result in results), f"Unexpected redirect statuses: {results[:3]}"
print(f"Public redirects verified: {len(results)}")