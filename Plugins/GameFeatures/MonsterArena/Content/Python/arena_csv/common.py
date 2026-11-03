import csv
from pathlib import Path

def read_rows(directory, filename, columns):
    with (Path(directory) / filename).open(encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != columns:
            raise ValueError(f"{filename}: expected columns {columns}")
        rows = list(reader)
    if not rows:
        raise ValueError(f"{filename}: table is empty")
    for line, row in enumerate(rows, 2):
        if None in row or any(value is None or not value.strip() for value in row.values()):
            raise ValueError(f"{filename}:{line}: missing or extra field")
    return rows



