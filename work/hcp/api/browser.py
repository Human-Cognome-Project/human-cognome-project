"""
Accessible browser: view websites the way a screen reader does.

Two modes:
1. Accessibility tree - structured semantic content (like a blind user navigates)
2. Screenshot - visual render for image processing

Uses Playwright with Chromium via the ARIA snapshot API.
"""
from __future__ import annotations

from pathlib import Path


def browse_accessible(url: str, wait_ms: int = 3000) -> str:
    """
    Browse a URL and return the accessibility tree as structured text.

    This is how a screen reader sees the page - structured,
    semantic, no visual noise. Returns ARIA snapshot format:
        - heading "Title" [level=1]
        - paragraph: Some text content
        - link "Click me":
          - /url: https://example.com
    """
    from playwright.sync_api import sync_playwright

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        page = browser.new_page()
        page.goto(url, wait_until="domcontentloaded")
        page.wait_for_timeout(wait_ms)

        snapshot = page.locator("body").aria_snapshot()

        browser.close()

    return snapshot


def browse_screenshot(
    url: str,
    output_path: str | Path = "/tmp/hcp_screenshot.png",
    width: int = 1280,
    height: int = 720,
    full_page: bool = False,
    wait_ms: int = 3000,
) -> Path:
    """
    Browse a URL and take a screenshot.

    Renders in a virtual head for visual processing.
    """
    from playwright.sync_api import sync_playwright

    output_path = Path(output_path)

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        page = browser.new_page(viewport={"width": width, "height": height})
        page.goto(url, wait_until="domcontentloaded")
        page.wait_for_timeout(wait_ms)

        page.screenshot(path=str(output_path), full_page=full_page)

        browser.close()

    return output_path


def browse_both(
    url: str,
    screenshot_path: str | Path = "/tmp/hcp_screenshot.png",
    wait_ms: int = 3000,
) -> tuple[str, Path]:
    """
    Browse a URL, return both accessible text and screenshot.

    Single page load for efficiency.
    """
    from playwright.sync_api import sync_playwright

    screenshot_path = Path(screenshot_path)

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        page = browser.new_page(viewport={"width": 1280, "height": 720})
        page.goto(url, wait_until="domcontentloaded")
        page.wait_for_timeout(wait_ms)

        snapshot = page.locator("body").aria_snapshot()
        page.screenshot(path=str(screenshot_path), full_page=False)

        browser.close()

    return snapshot, screenshot_path
