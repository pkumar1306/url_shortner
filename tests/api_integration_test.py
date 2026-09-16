import os
import sys
import uuid

try:
    import requests
except ImportError:
    print("Missing dependency: install requests with `python -m pip install requests`.", file=sys.stderr)
    raise SystemExit(2)

BASE_URL = os.environ.get("TEST_BASE_URL", "http://localhost:8080").rstrip("/")


def expect(condition, message):
    if not condition:
        raise AssertionError(message)


def issue_key():
    response = requests.post(f"{BASE_URL}/api/v1/keys", timeout=5)
    expect(response.status_code == 201, f"key issuance returned {response.status_code}: {response.text}")
    body = response.json()
    key = body.get("api_key")
    expect(isinstance(key, str) and len(key) == 64, "key response did not contain a 64-character API key")
    return key


def create_url(key):
    response = requests.post(
        f"{BASE_URL}/api/v1/urls",
        headers={"Authorization": f"Bearer {key}"},
        json={"url": f"https://example.com/{uuid.uuid4()}"},
        timeout=5,
    )
    expect(response.status_code == 201, f"authenticated creation returned {response.status_code}: {response.text}")
    body = response.json()
    expect(isinstance(body.get("code"), str) and body["code"], "creation response did not contain a code")
    return body["code"]


def main():
    owner_key = issue_key()
    other_key = issue_key()

    valid_body = {"url": "https://example.com/auth-check"}
    for headers in (
        {},
        {"Authorization": "Basic secret"},
        {"Authorization": "Bearer "},
        {"Authorization": "Bearer invalid-key"},
    ):
        response = requests.post(
            f"{BASE_URL}/api/v1/urls",
            headers=headers,
            json=valid_body,
            timeout=5,
        )
        expect(response.status_code == 401, f"invalid auth returned {response.status_code}: {response.text}")

    code = create_url(owner_key)
    other_code = create_url(other_key)
    expect(code != other_code, "two URL creations returned the same short code")

    owner_stats = requests.get(
        f"{BASE_URL}/api/v1/urls/{code}/stats",
        headers={"Authorization": f"Bearer {owner_key}"},
        timeout=5,
    )
    expect(owner_stats.status_code == 200, f"owner stats returned {owner_stats.status_code}: {owner_stats.text}")

    other_stats = requests.get(
        f"{BASE_URL}/api/v1/urls/{code}/stats",
        headers={"Authorization": f"Bearer {other_key}"},
        timeout=5,
    )
    expect(other_stats.status_code == 404, f"cross-owner stats returned {other_stats.status_code}: {other_stats.text}")

    redirect = requests.get(f"{BASE_URL}/{code}", allow_redirects=False, timeout=5)
    expect(redirect.status_code == 302, f"public redirect returned {redirect.status_code}: {redirect.text}")
    expect(redirect.headers.get("Location", "").startswith("https://example.com/"), "redirect Location was incorrect")

    print("API integration checks passed")


if __name__ == "__main__":
    try:
        main()
    except (AssertionError, requests.RequestException) as error:
        print(f"API integration checks failed: {error}", file=sys.stderr)
        raise SystemExit(1)
