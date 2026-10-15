from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import urllib.request,json,concurrent.futures
R=project_root(__file__);O=R/'reference/xwine1'
def get(url):
 q=urllib.request.Request(url,headers={'User-Agent':'FM5-readonly-source-audit','Accept':'application/vnd.github+json'});return json.load(urllib.request.urlopen(q,timeout=25))
repos=get('https://api.github.com/orgs/XWine1/repos?per_page=100&type=public');(O/'repositories.json').write_text(json.dumps(repos,indent=2))
for r in repos:
 print(json.dumps({k:r[k]for k in ['full_name','default_branch','archived','pushed_at','updated_at','size','description']}))
 name=r['name'];branch=r['default_branch'];tree=get(f'https://api.github.com/repos/XWine1/{name}/git/trees/{branch}?recursive=1');(O/f'{name}-tree.json').write_text(json.dumps(tree,indent=2))
 commits=get(f'https://api.github.com/repos/XWine1/{name}/commits?per_page=3');(O/f'{name}-commits.json').write_text(json.dumps(commits,indent=2))
 print('TREE',name,'sha',tree['sha'],'files',len(tree.get('tree',[])),'truncated',tree.get('truncated'))
 for x in tree.get('tree',[]):
  if x['type']=='blob':print(x['path'])
 print('LATEST',[(x['sha'],x['commit']['committer']['date'],x['commit']['message'].splitlines()[0])for x in commits])
