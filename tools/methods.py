import sys,re
lines=open(sys.argv[1],encoding='utf-8').read().split('\n')
cls=None
for i,l in enumerate(lines):
    m=re.match(r'^(\s*)DoInitAction sprite=\d+ (\S+)',l)
    if m: cls=m.group(2).replace('__Packages.',''); ind=m.group(1)+'  '; continue
    if cls and re.match('^'+ind+r'[ 0-9]{5} function',l):
        prev=lines[i-1].strip(); prev=re.sub(r'^\d+ Push ','',prev)
        print(cls,'::',prev,'|',l.strip().split('function',1)[1])
