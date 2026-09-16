import os
import sys

import requests

base_url = os.environ.get("TEST_BASE_URL", "http://localhost:8080").rstrip("/")
api_key = os.environ.get("TEST_API_KEY")
if not api_key:
    print("Set TEST_API_KEY before running this test.", file=sys.stderr)
    raise SystemExit(2)

url = f"{base_url}/api/v1/urls"

for i in range(21):
    response = requests.post(
        url,
        headers={"Authorization": f"Bearer {api_key}"},
        json={"url": f"https://example.com/{i}"}
    )

    print(i + 1, response.status_code, response.text)