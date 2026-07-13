#!/usr/bin/env python3

import argparse
import sys
import time
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


def test_webserver(url: str, expected_status: int, timeout: float) -> bool:
    start = time.perf_counter()
    request = Request(
        url,
        headers={"User-Agent": "webserver-test/1.0"},
        method="GET",
    )

    try:
        with urlopen(request, timeout=timeout) as response:
            body = response.read()
            elapsed_ms = (time.perf_counter() - start) * 1000
            status = response.status

            print(f"URL:          {url}")
            print(f"Status:       {status}")
            print(f"Response:     {len(body)} bytes")
            print(f"Content-Type: {response.headers.get('Content-Type', 'unknown')}")
            print(f"Time:         {elapsed_ms:.2f} ms")

            if status != expected_status:
                print(
                    f"FAIL: expected HTTP {expected_status}, received HTTP {status}",
                    file=sys.stderr,
                )
                return False

            print("PASS: webserver responded successfully")
            return True

    except HTTPError as error:
        print(f"FAIL: HTTP {error.code} - {error.reason}", file=sys.stderr)
    except URLError as error:
        print(f"FAIL: connection error - {error.reason}", file=sys.stderr)
    except TimeoutError:
        print(f"FAIL: request timed out after {timeout} seconds", file=sys.stderr)

    return False


def main() -> int:
    parser = argparse.ArgumentParser(description="Test a webserver endpoint.")
    parser.add_argument(
        "url",
        nargs="?",
        default="http://localhost:8080/",
        help="URL to test (default: http://localhost:8080/)",
    )
    parser.add_argument(
        "--status",
        type=int,
        default=200,
        help="Expected HTTP status code (default: 200)",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=5.0,
        help="Request timeout in seconds (default: 5)",
    )
    args = parser.parse_args()

    return 0 if test_webserver(args.url, args.status, args.timeout) else 1


if __name__ == "__main__":
    raise SystemExit(main())