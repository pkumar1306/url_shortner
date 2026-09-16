import os
import sys
from concurrent.futures import ThreadPoolExecutor

import requests

BASE_URL = os.environ.get("TEST_BASE_URL", "http://localhost:8080").rstrip("/")
API_KEY = os.environ.get("TEST_API_KEY")
if not API_KEY:
    print("Set TEST_API_KEY before running this test.", file=sys.stderr)
    raise SystemExit(2)

URL = f"{BASE_URL}/api/v1/urls"

def create_url(i):
    response = requests.post(
        URL,
        headers={"Authorization": f"Bearer {API_KEY}"},
        json={"url": f"https://example.com/page/{i}"}
    )

    return response.status_code, response.json()


with ThreadPoolExecutor(max_workers=20) as executor:
    results = list(executor.map(create_url, range(100)))


success = [r for r in results if r[0] == 201]
rate_limited = [r for r in results if r[0] == 429]
unexpected = [r for r in results if r[0] not in (201, 429)]

assert not unexpected, f"Unexpected responses: {unexpected[:3]}"
assert success, "No authenticated URL creation succeeded"

codes = [r[1]["code"] for r in success]

assert len(codes) == len(set(codes)), "Duplicate short codes were returned"
print(f"Total requests: {len(results)}")
print(f"Successful: {len(success)}")
print(f"Rate limited: {len(rate_limited)}")
print("Unique short codes verified")