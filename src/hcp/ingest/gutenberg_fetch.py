"""
Gutenberg text fetcher using Gutendex API.

Downloads texts with rich metadata for PBM encoding experiments.
"""

import requests
import json
from pathlib import Path
from typing import Dict, List, Optional
from dataclasses import dataclass, asdict
import time


@dataclass
class GutenbergBook:
    """Metadata for a Gutenberg book."""
    id: int
    title: str
    authors: List[Dict[str, any]]
    subjects: List[str]
    bookshelves: List[str]
    languages: List[str]
    copyright: Optional[bool]
    download_count: int
    formats: Dict[str, str]

    @property
    def plain_text_url(self) -> Optional[str]:
        """Get plain text URL if available."""
        # Prefer UTF-8 plain text
        for key in self.formats:
            if 'text/plain' in key and 'utf-8' in key.lower():
                return self.formats[key]
        # Fallback to any plain text
        for key in self.formats:
            if 'text/plain' in key:
                return self.formats[key]
        return None

    @property
    def author_names(self) -> List[str]:
        """Extract author names."""
        return [author['name'] for author in self.authors]

    @property
    def author_birth_years(self) -> List[int]:
        """Extract author birth years if available."""
        years = []
        for author in self.authors:
            if 'birth_year' in author and author['birth_year']:
                years.append(author['birth_year'])
        return years


class GutendexFetcher:
    """Fetch books from Gutendex API with filtering."""

    BASE_URL = "https://gutendex.com/books"

    def __init__(self, rate_limit: float = 1.0):
        """
        Initialize fetcher.

        Args:
            rate_limit: Seconds to wait between API calls
        """
        self.rate_limit = rate_limit

    def fetch_books(
        self,
        language: Optional[str] = None,
        topic: Optional[str] = None,
        author_year_start: Optional[int] = None,
        author_year_end: Optional[int] = None,
        copyright: Optional[bool] = None,
        search: Optional[str] = None,
        sort: str = "popular",
        max_books: Optional[int] = None
    ) -> List[GutenbergBook]:
        """
        Fetch books with filtering.

        Args:
            language: Language code (e.g., 'en', 'fr')
            topic: Topic/subject filter (e.g., 'fiction', 'history')
            author_year_start: Minimum author birth year
            author_year_end: Maximum author birth year
            copyright: Filter by copyright status (False = public domain)
            search: Search term for title/author
            sort: Sort order ('popular', 'ascending', 'descending')
            max_books: Maximum number of books to fetch (None = all)

        Returns:
            List of GutenbergBook objects
        """
        books = []
        page = 1

        while True:
            # Build query parameters
            params = {
                'page': page,
                'sort': sort
            }

            if language:
                params['languages'] = language
            if topic:
                params['topic'] = topic
            if author_year_start:
                params['author_year_start'] = author_year_start
            if author_year_end:
                params['author_year_end'] = author_year_end
            if copyright is not None:
                params['copyright'] = str(copyright).lower()
            if search:
                params['search'] = search

            # Fetch page
            print(f"Fetching page {page}...")
            response = requests.get(self.BASE_URL, params=params)
            response.raise_for_status()

            data = response.json()
            results = data['results']

            if not results:
                break

            # Parse results
            for book_data in results:
                book = GutenbergBook(
                    id=book_data['id'],
                    title=book_data['title'],
                    authors=book_data['authors'],
                    subjects=book_data.get('subjects', []),
                    bookshelves=book_data.get('bookshelves', []),
                    languages=book_data['languages'],
                    copyright=book_data.get('copyright'),
                    download_count=book_data.get('download_count', 0),
                    formats=book_data['formats']
                )
                books.append(book)

                if max_books and len(books) >= max_books:
                    return books

            # Check if more pages
            if not data['next']:
                break

            page += 1
            time.sleep(self.rate_limit)  # Rate limiting

        return books

    def download_text(self, book: GutenbergBook, output_dir: Path) -> Optional[Path]:
        """
        Download plain text for a book.

        Args:
            book: GutenbergBook object
            output_dir: Directory to save text file

        Returns:
            Path to downloaded file, or None if unavailable
        """
        url = book.plain_text_url
        if not url:
            print(f"No plain text available for {book.id}: {book.title}")
            return None

        output_dir.mkdir(parents=True, exist_ok=True)

        # Sanitize filename
        safe_title = "".join(c for c in book.title if c.isalnum() or c in (' ', '-', '_'))
        safe_title = safe_title[:100]  # Limit length

        filename = f"{book.id:05d}_{safe_title}.txt"
        filepath = output_dir / filename

        print(f"Downloading {book.id}: {book.title}...")
        response = requests.get(url)
        response.raise_for_status()

        # Detect encoding
        encoding = response.encoding or 'utf-8'

        filepath.write_text(response.text, encoding='utf-8')
        print(f"  Saved to {filepath}")

        return filepath

    def download_collection(
        self,
        books: List[GutenbergBook],
        output_dir: Path,
        metadata_file: Optional[Path] = None
    ) -> List[Path]:
        """
        Download multiple books and save metadata.

        Args:
            books: List of GutenbergBook objects
            output_dir: Directory to save texts
            metadata_file: Optional path to save metadata JSON

        Returns:
            List of paths to downloaded files
        """
        downloaded = []

        for book in books:
            filepath = self.download_text(book, output_dir)
            if filepath:
                downloaded.append(filepath)
            time.sleep(self.rate_limit)  # Rate limiting

        # Save metadata
        if metadata_file:
            metadata = [asdict(book) for book in books]
            metadata_file.write_text(json.dumps(metadata, indent=2))
            print(f"\nMetadata saved to {metadata_file}")

        print(f"\nDownloaded {len(downloaded)} / {len(books)} books")
        return downloaded


# Example usage functions

def fetch_english_fiction_sample(max_books: int = 100) -> List[GutenbergBook]:
    """Fetch sample of popular English fiction."""
    fetcher = GutendexFetcher()
    return fetcher.fetch_books(
        language='en',
        topic='fiction',
        copyright=False,  # Public domain only
        sort='popular',
        max_books=max_books
    )


def fetch_by_era(start_year: int, end_year: int, max_books: int = 50) -> List[GutenbergBook]:
    """Fetch books by author birth year range."""
    fetcher = GutendexFetcher()
    return fetcher.fetch_books(
        language='en',
        author_year_start=start_year,
        author_year_end=end_year,
        copyright=False,
        max_books=max_books
    )


def fetch_multilingual_sample(languages: List[str], max_per_lang: int = 20) -> Dict[str, List[GutenbergBook]]:
    """Fetch samples in multiple languages."""
    fetcher = GutendexFetcher()
    result = {}

    for lang in languages:
        print(f"\nFetching {lang} books...")
        books = fetcher.fetch_books(
            language=lang,
            copyright=False,
            max_books=max_per_lang
        )
        result[lang] = books

    return result


if __name__ == "__main__":
    # Example: Download 10 popular English novels
    from pathlib import Path

    output_dir = Path("data/gutenberg/texts")
    metadata_file = Path("data/gutenberg/metadata.json")

    fetcher = GutendexFetcher()
    books = fetch_english_fiction_sample(max_books=10)

    print(f"\nFound {len(books)} books")
    print("\nSample titles:")
    for book in books[:5]:
        print(f"  {book.id}: {book.title} by {', '.join(book.author_names)}")

    # Download
    fetcher.download_collection(books, output_dir, metadata_file)
