import hashlib
import json

def stable_jsonDumps(obj):
    '''固定顺序序列化，保证哈希值稳定'''
    return json.dumps(obj, sort_keys=True, separators=(',', ':'))

def md5_hash(s: str) -> str:
    return hashlib.md5(s.encode('utf-8')).hexdigest()

def hash_skillData(skill_json: dict, script_content: str) -> str:
    '''计算技能数据的hash'''
    sj = dict(skill_json)
    sj.pop("last_modified", None) # 排除 last_modified 字段
    sj.pop("_meta", None) # 排除 _meta 字段
    combined = stable_jsonDumps(sj) + "\n--SCRIPT--\n" + script_content
    return md5_hash(combined)

