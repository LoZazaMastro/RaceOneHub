"""Source-only synthetic PE failure/overlap tests. No owner input is required."""
from pathlib import Path
import hashlib,json,struct,tempfile,unittest
from unittest.mock import patch
import fm5_bridge_template as tool


def image(changes):
 raw=bytearray(0x60200);raw[:2]=b'MZ';struct.pack_into('<I',raw,0x3c,0x80);raw[0x80:0x84]=b'PE\0\0'
 struct.pack_into('<HHIIIHH',raw,0x84,0x8664,1,0,0,0,0xf0,0x2022);o=0x98
 struct.pack_into('<H',raw,o,0x20b);struct.pack_into('<Q',raw,o+24,0x180000000);struct.pack_into('<II',raw,o+32,0x1000,0x200);struct.pack_into('<II',raw,o+56,0x61000,0x200);struct.pack_into('<I',raw,o+108,16)
 s=o+0xf0;raw[s:s+8]=b'.rdata\0\0';struct.pack_into('<IIII',raw,s+8,0x60000,0x1000,0x60000,0x200);struct.pack_into('<I',raw,s+36,0x40000040)
 for rva,data in changes.items():raw[0x200+rva-0x1000:0x200+rva-0x1000+len(data)]=data
 return raw

class TemplateTests(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory(prefix='raceone-bridge-fixture-');self.root=Path(self.temp.name);self.inc=self.root/'includes';self.inc.mkdir();(self.root/'owner-game'/'Mount').mkdir(parents=True);self.game=self.root/'owner-game'/'Mount'/'game.exe';self.game.write_bytes(b'owner placeholder')
  self.values={};changes={};cursor=0x2000
  for name in tool.ORIGINS:
   size=110 if name=='guard_host_locale'else 38 if name=='guard_host_set_locale'else 8 if name=='guard_initial_buffer'else 256
   value=(hashlib.sha256(name.encode()).digest()*20)[:size]
   if name=='guard_initial_buffer':value=bytes(8)
   self.values[name]=value;changes[cursor]=value
   if name=='guard_initial_buffer':self.zero_rva=cursor
   cursor+=(size+15)&~15
  # A semantic chunk also occurs inside another full shader: both must zero,
  # and the overlapping reconstruction must agree byte for byte.
  self.values['original_11d41d0_osgn']=self.values['fm5_resolve_four_samples'][24:68]
  changes[cursor]=self.values['original_11d41d0_osgn'];cursor+=0x100
  changes[cursor]=struct.pack('<I4xQQ',0x25d293d,8,0x180000000+self.zero_rva)
  self.bridge=self.root/'Bridge.dll';self.bridge.write_bytes(image(changes));self.host=self.root/'Host.exe';self.host.write_bytes(image({start:self.values[name]for name,(start,end)in tool.HOST_RANGES.items()}))
  for filename in tool.INCLUDES:(self.inc/filename).write_text('')
  (self.inc/tool.INCLUDES[0]).write_text(''.join(tool.recipe.array(name,data)for name,data in self.values.items()))
  (self.inc/'generation-report.json').write_text(json.dumps({'array_count':28,'input_sha256':{'game':tool.recipe.GAME_SHA,'engine':tool.recipe.ENGINE_SHA},'arrays':[{'name':name,'bytes':len(data),'sha256':tool.sha(data)}for name,data in self.values.items()]}))
  self.stage=self.root/'template';tool.create_template(self.bridge,self.inc,self.host,self.stage);self.template=self.stage/'ForzaMastro5Bridge.template';self.manifest=self.stage/'manifest.json';self.output=self.root/'output.dll'
 def tearDown(self):self.temp.cleanup()
 def run_hydrate(self):
  with patch.object(tool,'_regenerate',return_value=self.values):return tool.hydrate_bridge(self.template,self.manifest,self.game,self.host,self.output)
 def edit_manifest(self,edit):
  z=json.loads(self.manifest.read_text());edit(z);self.manifest.write_text(json.dumps(z))
 def test_all28_zero_no_private_nonzero_array_survives(self):
  raw=self.template.read_bytes();z=json.loads(self.manifest.read_text());self.assertEqual(z['array_count'],28)
  for item in z['arrays']:
   for offset in item['offsets']:self.assertEqual(raw[offset:offset+item['bytes']],bytes(item['bytes']))
  for value in self.values.values():
   if any(value):self.assertNotIn(value,raw)
  self.assertFalse(z['ready_to_load']);self.assertNotIn(str(self.root),self.manifest.read_text())
 def test_exact_roundtrip_and_overlaps(self):
  report=self.run_hydrate();self.assertEqual(self.output.read_bytes(),self.bridge.read_bytes());self.assertEqual(report['arrays'],28);self.assertFalse(report['game_executed'])
  z=json.loads(self.manifest.read_text());record=next(r for r in z['arrays']if r['name']=='original_11d41d0_osgn');self.assertGreaterEqual(len(record['offsets']),2)
 def test_existing_output_is_preserved(self):
  self.output.write_bytes(b'keep')
  with self.assertRaises(ValueError):self.run_hydrate()
  self.assertEqual(self.output.read_bytes(),b'keep')
 def test_template_corruption(self):
  b=bytearray(self.template.read_bytes());b[-1]^=1;self.template.write_bytes(b)
  with self.assertRaises(ValueError):self.run_hydrate()
  self.assertFalse(self.output.exists())
 def test_wrong_origin(self):
  self.edit_manifest(lambda z:z['arrays'][0]['origin'].update(rva=1))
  with self.assertRaises(ValueError):self.run_hydrate()
 def test_offset_in_pe_header_refused(self):
  self.edit_manifest(lambda z:z['arrays'][0].update(offsets=[0]))
  with self.assertRaises(ValueError):self.run_hydrate()
 def test_overlap_disagreement_refused(self):
  self.edit_manifest(lambda z:z['arrays'][1].update(offsets=z['arrays'][0]['offsets']))
  with self.assertRaisesRegex(ValueError,'Overlapping'):self.run_hydrate()
  self.assertFalse(self.output.exists())
 def test_wrong_hydrated_digest_refused(self):
  self.edit_manifest(lambda z:z.update(hydrated_sha256='0'*64))
  with self.assertRaisesRegex(ValueError,'baseline'):self.run_hydrate()
  self.assertFalse(self.output.exists());self.assertEqual(list(self.root.glob('.bridge-hydrate-*')),[])
 def test_recovered_array_digest_checked(self):
  self.edit_manifest(lambda z:z['arrays'][0].update(sha256='0'*64))
  with self.assertRaisesRegex(ValueError,'Locally recovered'):self.run_hydrate()
  self.assertFalse(self.output.exists())
 def test_wrong_game_input_rejected(self):
  with patch.object(tool.recipe,'checked_input',side_effect=ValueError('wrong game')):
   with self.assertRaisesRegex(ValueError,'wrong game'):tool.hydrate_bridge(self.template,self.manifest,self.game,self.host,self.output)
  self.assertFalse(self.output.exists())
 def test_derived_array_in_executable_section_rejected(self):
  b=bytearray(self.bridge.read_bytes());struct.pack_into('<I',b,0x98+0xf0+36,0x60000040);self.bridge.write_bytes(b)
  with self.assertRaisesRegex(ValueError,'executable'):tool.create_template(self.bridge,self.inc,self.host,self.root/'bad-template')
 def test_prepared_host_changed_guard_refused(self):
  b=bytearray(self.host.read_bytes());b[0x200+0x37d00-0x1000]^=1;self.host.write_bytes(b)
  with self.assertRaisesRegex(ValueError,'locale guards'):tool.create_template(self.bridge,self.inc,self.host,self.root/'bad-template')
 def edit_report(self,inputs):
  path=self.inc/'generation-report.json';z=json.loads(path.read_text());z['input_sha256']=inputs;path.write_text(json.dumps(z))
 def test_prepared_host_provenance_same_template(self):
  self.edit_report({'game':tool.recipe.GAME_SHA,'prepared_host':'1'*64});other=self.root/'host-template'
  tool.create_template(self.bridge,self.inc,self.host,other)
  self.assertEqual((other/'ForzaMastro5Bridge.template').read_bytes(),self.template.read_bytes());self.assertEqual((other/'manifest.json').read_text(),self.manifest.read_text())
 def test_unknown_provenance_refused(self):
  for inputs in[{'game':'0'*64,'prepared_host':'1'*64},{'game':tool.recipe.GAME_SHA,'engine':'0'*64},{'game':tool.recipe.GAME_SHA,'engine':tool.recipe.ENGINE_SHA,'prepared_host':'1'*64}]:
   self.edit_report(inputs)
   with self.assertRaisesRegex(ValueError,'provenance'):tool.create_template(self.bridge,self.inc,self.host,self.root/'bad-template')
 def test_generator_refuses_host_without_original_guards(self):
  with self.assertRaisesRegex(ValueError,'does not retain'):tool.recipe.checked_host(self.host)
if __name__=='__main__':unittest.main()
