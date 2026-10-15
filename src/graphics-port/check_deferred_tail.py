import json
j=json.load(open('analysis/graphics-port/native-context-tables-comparison.json'));t=next(t for t in j[0]['tables']if t['rva']=='0x8e450');print([(hex(i),t['entries'][i])for i in [0x92,0x93,0x94,0x95,0x96,0x97,0x98,0xc1,0xc2]])
