"""
宠物数据导入器
从 CSV 导入宠物基础数据与可学技能映射到 SQLite，校验失败则拒绝导入。
"""

import argparse
import csv
import sqlite3
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# 允许的属性映射（英文 + 中文），统一存储为英文名称。
ATTR_ALIASES = {
    "normal": "Normal",
    "火": "Fire",
    "fire": "Fire",
    "水": "Water",
    "water": "Water",
    "电": "Electric",
    "electric": "Electric",
    "草": "Grass",
    "grass": "Grass",
    "冰": "Ice",
    "ice": "Ice",
    "武": "Fighting",
    "fighting": "Fighting",
    "毒": "Poison",
    "poison": "Poison",
    "土": "Ground",
    "ground": "Ground",
    "翼": "Flying",
    "flying": "Flying",
    "萌": "Cute",
    "cute": "Cute",
    "虫": "Worm",
    "worm": "Worm",
    "石": "Rock",
    "rock": "Rock",
    "幽": "Ghost",
    "ghost": "Ghost",
    "龙": "Dragon",
    "dragon": "Dragon",
    "恶魔": "Demon",
    "demon": "Demon",
    "机械": "Steel",
    "steel": "Steel",
    "光": "Light",
    "light": "Light",
    "神火": "dFire",
    "dfire": "dFire",
    "神草": "dGrass",
    "dgrass": "dGrass",
    "神水": "dWater",
    "dwater": "dWater",
    "none": "None",
    "": "None",
}


@dataclass
class PetRow:
    id: int
    name: str
    attr1: str
    attr2: str
    bEne: int
    bAtk: int
    bDef: int
    bSpA: int
    bSpD: int
    bSpe: int
    source_row: int


@dataclass(frozen=True)
class PetSkillRow:
    pet_id: int
    skill_id: int
    source_row: int


class PetImporter:
    def __init__(self, db_path: Optional[Path] = None):
        root_dir = Path(__file__).resolve().parents[2]
        base_dir = root_dir / "assets" / "pets"
        base_dir.mkdir(parents=True, exist_ok=True)
        self.db_path = db_path or (base_dir / "pets.db")

    def _parse_attr(self, raw: str, row_no: int, col: str, errors: List[str]) -> Optional[str]:
        key = (raw or "").strip()
        key_lower = key.lower()
        if key_lower not in ATTR_ALIASES:
            errors.append(f"Row {row_no}: invalid attr '{raw}' in column '{col}'")
            return None
        return ATTR_ALIASES[key_lower]

    def _parse_int(self, raw, row_no: int, col: str, errors: List[str]) -> Optional[int]:
        try:
            val = int(raw)
            if val < 0:
                errors.append(f"Row {row_no}: negative value in '{col}'")
                return None
            return val
        except (TypeError, ValueError):
            errors.append(f"Row {row_no}: invalid integer in '{col}' -> '{raw}'")
            return None

    def load_pets_csv(self, path: Path, encoding: str = "utf-8") -> Tuple[List[PetRow], List[str]]:
        errors: List[str] = []
        pets: List[PetRow] = []
        try:
            with open(path, "r", encoding=encoding, newline="") as f:
                reader = csv.DictReader(f)
                required = {"id", "name", "attr1", "bEne", "bAtk", "bDef", "bSpA", "bSpD", "bSpe"}
                missing_cols = required - set(reader.fieldnames or [])
                if missing_cols:
                    return [], [f"Pets CSV missing columns: {', '.join(sorted(missing_cols))}"]

                for idx, row in enumerate(reader, start=2):
                    row_errors: List[str] = []
                    pet_id = self._parse_int(row.get("id"), idx, "id", row_errors)
                    name = (row.get("name") or "").strip()
                    if not name:
                        row_errors.append(f"Row {idx}: name is required")
                    attr1 = self._parse_attr(row.get("attr1", ""), idx, "attr1", row_errors)
                    attr2_raw = row.get("attr2", "")
                    attr2 = self._parse_attr(attr2_raw, idx, "attr2", row_errors) if attr2_raw is not None else "None"
                    bEne = self._parse_int(row.get("bEne"), idx, "bEne", row_errors)
                    bAtk = self._parse_int(row.get("bAtk"), idx, "bAtk", row_errors)
                    bDef = self._parse_int(row.get("bDef"), idx, "bDef", row_errors)
                    bSpA = self._parse_int(row.get("bSpA"), idx, "bSpA", row_errors)
                    bSpD = self._parse_int(row.get("bSpD"), idx, "bSpD", row_errors)
                    bSpe = self._parse_int(row.get("bSpe"), idx, "bSpe", row_errors)

                    if row_errors:
                        errors.extend(row_errors)
                        continue

                    pets.append(
                        PetRow(
                            id=pet_id,  # type: ignore[arg-type]
                            name=name,
                            attr1=attr1 or "None",
                            attr2=attr2 or "None",
                            bEne=bEne,  # type: ignore[arg-type]
                            bAtk=bAtk,  # type: ignore[arg-type]
                            bDef=bDef,  # type: ignore[arg-type]
                            bSpA=bSpA,  # type: ignore[arg-type]
                            bSpD=bSpD,  # type: ignore[arg-type]
                            bSpe=bSpe,  # type: ignore[arg-type]
                            source_row=idx,
                        )
                    )
        except FileNotFoundError:
            errors.append(f"Pets CSV not found: {path}")
        except UnicodeDecodeError:
            errors.append(f"Encoding error reading pets CSV: try --encoding utf-8-sig / gbk")
        return pets, errors

    def load_pet_skills_csv(self, path: Path, encoding: str = "utf-8") -> Tuple[List[PetSkillRow], List[str]]:
        errors: List[str] = []
        rows: List[PetSkillRow] = []
        try:
            with open(path, "r", encoding=encoding, newline="") as f:
                reader = csv.DictReader(f)
                required = {"pet_id", "skill_id"}
                missing_cols = required - set(reader.fieldnames or [])
                if missing_cols:
                    return [], [f"Pet skills CSV missing columns: {', '.join(sorted(missing_cols))}"]

                for idx, row in enumerate(reader, start=2):
                    row_errors: List[str] = []
                    pet_id = self._parse_int(row.get("pet_id"), idx, "pet_id", row_errors)
                    skill_id = self._parse_int(row.get("skill_id"), idx, "skill_id", row_errors)
                    if row_errors:
                        errors.extend(row_errors)
                        continue
                    rows.append(PetSkillRow(pet_id=pet_id, skill_id=skill_id, source_row=idx))  # type: ignore[arg-type]
        except FileNotFoundError:
            errors.append(f"Pet skills CSV not found: {path}")
        except UnicodeDecodeError:
            errors.append(f"Encoding error reading pet skills CSV: try --encoding utf-8-sig / gbk")
        return rows, errors

    def validate(self, pets: List[PetRow], pet_skills: List[PetSkillRow]) -> List[str]:
        errors: List[str] = []
        seen_pet_ids = set()
        for p in pets:
            if p.id in seen_pet_ids:
                errors.append(f"Duplicate pet id {p.id} (row {p.source_row})")
            seen_pet_ids.add(p.id)

        pet_id_set = {p.id for p in pets}

        skill_pairs: Dict[Tuple[int, int], int] = {}
        for rel in pet_skills:
            if rel.pet_id not in pet_id_set:
                errors.append(f"Row {rel.source_row} in pet skills: pet_id {rel.pet_id} not defined in pets CSV")
            key = (rel.pet_id, rel.skill_id)
            if key in skill_pairs:
                errors.append(
                    f"Row {rel.source_row} in pet skills duplicates pair pet_id={rel.pet_id}, skill_id={rel.skill_id}"
                )
            skill_pairs[key] = rel.source_row
        return errors

    def init_db(self, conn: sqlite3.Connection):
        conn.execute("PRAGMA foreign_keys = ON;")
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS pets(
                id INTEGER PRIMARY KEY,
                name TEXT NOT NULL,
                attr1 TEXT NOT NULL,
                attr2 TEXT NOT NULL,
                bEne INTEGER NOT NULL,
                bAtk INTEGER NOT NULL,
                bDef INTEGER NOT NULL,
                bSpA INTEGER NOT NULL,
                bSpD INTEGER NOT NULL,
                bSpe INTEGER NOT NULL
            );
            """
        )
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS pet_skills(
                pet_id INTEGER NOT NULL,
                skill_id INTEGER NOT NULL,
                PRIMARY KEY(pet_id, skill_id),
                FOREIGN KEY(pet_id) REFERENCES pets(id) ON DELETE CASCADE
            );
            """
        )

    def import_data(self, pets: List[PetRow], pet_skills: List[PetSkillRow]) -> Dict[str, int]:
        with sqlite3.connect(self.db_path) as conn:
            self.init_db(conn)
            conn.execute("BEGIN;")
            conn.execute("DELETE FROM pet_skills;")
            conn.execute("DELETE FROM pets;")
            conn.executemany(
                """
                INSERT INTO pets(id, name, attr1, attr2, bEne, bAtk, bDef, bSpA, bSpD, bSpe)
                VALUES(:id, :name, :attr1, :attr2, :bEne, :bAtk, :bDef, :bSpA, :bSpD, :bSpe);
                """,
                [p.__dict__ for p in pets],
            )
            conn.executemany(
                """
                INSERT INTO pet_skills(pet_id, skill_id)
                VALUES(:pet_id, :skill_id);
                """,
                [rel.__dict__ for rel in pet_skills],
            )
            conn.commit()
        return {"pets": len(pets), "pet_skills": len(pet_skills)}


def main():
    parser = argparse.ArgumentParser(description="Import pet data and learnable skills into SQLite")
    parser.add_argument("--pets-csv", required=True, help="CSV path for pet base data")
    parser.add_argument("--pet-skills-csv", required=True, help="CSV path for pet-skill relations")
    parser.add_argument("--db-path", help="Output SQLite path (default: assets/pets/pets.db)")
    parser.add_argument("--encoding", default="utf-8", help="CSV encoding (default: utf-8)")
    args = parser.parse_args()

    importer = PetImporter(db_path=Path(args.db_path) if args.db_path else None)

    pets, pet_errors = importer.load_pets_csv(Path(args.pets_csv), encoding=args.encoding)
    rels, skill_errors = importer.load_pet_skills_csv(Path(args.pet_skills_csv), encoding=args.encoding)

    errors = pet_errors + skill_errors
    if not pet_errors:
        errors.extend(importer.validate(pets, rels))

    if errors:
        print("✗ Import aborted due to errors:")
        for err in errors:
            print(f"  - {err}")
        return

    counts = importer.import_data(pets, rels)
    print(f"✓ Imported {counts['pets']} pets and {counts['pet_skills']} pet-skill relations into {importer.db_path}")


if __name__ == "__main__":
    main()
