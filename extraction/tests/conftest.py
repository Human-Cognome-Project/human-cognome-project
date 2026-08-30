"""Shared fixtures and markers for HCP tests."""

import pytest


def pytest_configure(config):
    config.addinivalue_line("markers", "db: requires PostgreSQL connection")
