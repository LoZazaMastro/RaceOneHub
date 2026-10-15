import json
j=json.load(open('analysis/graphics-port/native-context-tables-comparison.json'));t=next(t for t in j[1]['tables']if t['rva']=='0xd6950');print([(hex(i),x)for i,x in enumerate(t['entries'][:0x28])])
