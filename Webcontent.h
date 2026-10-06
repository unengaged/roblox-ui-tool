#pragma once
static const char kIndexHtml[] = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>texture replacer</title>
<style>
@font-face{
  font-family:'Angel Bunny';
  src:url('angelbunny.ttf') format('truetype');
  font-weight:normal; font-style:normal; font-display:swap;
}
:root{
  --black:#000;
  --white:#fff;
  --gray:#555;
  --radius-lg:34px;
  --radius-md:26px;
  --radius-sm:10px;
  --line:4px;
  --ui-scale:0.75;

  --frame:url('https://file.garden/ZjCpntlBORniyXDK/border.png');
  --frame-slice:33%;
  --frame-width:64px;
  --frame-gap:30px;
  box-sizing:border-box;
  padding-top:env(safe-area-inset-top,0px);
  padding-bottom:env(safe-area-inset-bottom,0px);
}
html{scroll-padding-top:env(safe-area-inset-top,0px)}
*,*::before,*::after{box-sizing:border-box}
*{-webkit-user-select:none; user-select:none; -webkit-user-drag:none}
.foot{-webkit-user-select:text; user-select:text}
body{
  margin:0; background:var(--black); color:#fff;
  font-family:'Angel Bunny',ui-rounded,'SF Pro Rounded','Nunito','Segoe UI',system-ui,sans-serif;
  min-height:100vh; display:flex; flex-direction:column; align-items:center; gap:18px; padding:20px 14px 28px;
}
.shell{width:100%; max-width:520px; background:var(--black); color:var(--white); border-radius:44px; padding:22px 20px 24px}
.app{width:100%; display:flex; flex-direction:column; gap:16px}
h1,h2,h3{font-weight:normal}
h1{
  margin:14px 0 4px; text-align:center; font-size:clamp(34px,8.5vw,48px); letter-spacing:.01em; line-height:1;
  background:linear-gradient(to bottom,var(--black) 40%,var(--white));
  -webkit-background-clip:text; background-clip:text;
  color:transparent; -webkit-text-fill-color:transparent;
  -webkit-text-stroke:2px var(--white);
  animation:titlepop 1s ease-in-out infinite alternate;
}
@keyframes titlepop{
  from{transform:scale(.98)}
  50%{transform:scale(1)}
  to{transform:scale(.98)}
}
.panel{
  border:var(--line) solid var(--black); border-radius:var(--radius-sm);
  --fill:var(--white); padding:16px;
}
button{font:inherit; color:inherit; cursor:pointer}
:focus-visible{outline:3px dashed var(--gray); outline-offset:3px}

.modes{display:grid; grid-template-columns:1fr 1fr; gap:11px}
.mode{
  border:3px solid var(--black); border-radius:var(--radius-md);
  background:var(--white); padding:8px 4px; font-size:clamp(11px,2.8vw,15px); white-space:nowrap;   transition:background .15s, transform .1s;
}
.mode:active{transform:scale(.98)}
.mode[aria-pressed="true"]{background:var(--black); color:var(--white)}

.main{display:block}
.drop{
  position:relative; aspect-ratio:1/1; width:100%;
  border:var(--line) solid var(--black); border-radius:var(--radius-sm);
  --fill:var(--white); display:flex; align-items:center; justify-content:center;
  text-align:center; font-size:clamp(16px,4vw,22px); overflow:hidden; padding:0;
  transition:background .15s;
}
.drop:hover,.drop.over{--fill:var(--black); color:var(--white)}
.drop img{position:absolute; inset:0; width:100%; height:100%; object-fit:contain; background:
  conic-gradient(#0000 25%,#0001 0 50%,#0000 0 75%,#0001 0) 0 0/20px 20px}
.drop{flex-direction:column}
.drop .hint{position:relative; padding:10px 10px 2px}
.drop .desc{position:relative; font-size:12px; opacity:.7; padding:0 10px 10px}
.drop .swap{
  position:absolute; bottom:8px; left:50%; transform:translateX(-50%);
  background:var(--white); border:3px solid var(--black); border-radius:999px;
  padding:2px 12px; font-size:13px;
}

.side{display:flex; flex-direction:column; min-width:0}
.side h2{
  margin:4px 0 8px; text-align:center; font-size:clamp(16px,4vw,22px);
}
.divider{display:block; width:100%; height:auto; filter:drop-shadow(0 0 4px rgba(0,0,0,.45))}
.targets{
  margin-top:14px; flex:1; border:3px solid var(--black); border-radius:var(--radius-sm);
  padding:8px 10px; display:flex; flex-direction:column; justify-content:space-around; gap:2px;
}
.row{
  display:flex; align-items:center; gap:12px; background:none; border:0; text-align:left;
  padding:8px 18px; border-radius:16px; width:100%; font-size:clamp(17px,4.4vw,26px); }
.row:hover{background:var(--black); color:var(--white)}
.box{
  flex:none; width:24px; height:24px; border:3px solid var(--black); border-radius:7px;
  display:grid; place-items:center; background:var(--white);
}
.box[aria-checked="true"]{background:var(--black); color:var(--white)}
.row:hover .box{border-color:var(--white)}
.box svg{width:14px; height:14px; opacity:0; fill:currentColor; stroke:none}
.box[aria-checked="true"] svg{opacity:1}

.replace{
  width:100%; border:var(--line) solid var(--black); border-radius:var(--radius-sm); --fill:var(--white);
  padding:20px; font-size:clamp(18px,4.6vw,24px); transition:background .15s, transform .1s;
}
.replace:active{transform:scale(.99)}

dialog{
  width:min(92vw,360px); padding:18px; margin:auto; color:var(--black); --fill:var(--white);
  border:var(--line) solid var(--black); border-radius:var(--radius-sm);
}
dialog::backdrop{background:rgba(0,0,0,.6)}
dialog[open]{animation:pop .18s ease-out}
@keyframes pop{from{transform:scale(.94); opacity:0}to{transform:none; opacity:1}}
@media (prefers-reduced-motion:reduce){dialog[open]{animation:none}}
.dlg-head{display:flex; align-items:center; justify-content:space-between; margin-bottom:12px}
.dlg-head h3{margin:0; font-size:26px; }
.x{width:40px; height:40px; border:3px solid var(--black); border-radius:var(--radius-sm); --fill:var(--white); font-size:20px; line-height:1}
.x:hover{--fill:var(--black); color:var(--white)}
dialog .drop{aspect-ratio:16/10; border-radius:var(--radius-sm)}
.dlg-actions{display:flex; gap:10px; margin-top:16px}
.dlg-actions button{
  flex:1; border:var(--line) solid var(--black); border-radius:var(--radius-sm); padding:12px; font-size:18px; --fill:var(--white)
}
.dlg-actions .primary{--fill:var(--black); color:var(--white)}
.dlg-actions button:hover{--fill:var(--black); color:var(--white)}
.dlg-actions .primary:hover{--fill:var(--white); color:var(--black)}

@media (max-width:480px){
  .panel{padding:12px}
}
input[type=file]{display:none}
.foot{font-size:14px; text-align:center; color:#fff; opacity:.85}

.targets,.drop,.x,.dlg-actions button,dialog{background:var(--fill)}

.shell{
  border:var(--frame-gap) solid transparent;
  border-image:var(--frame) var(--frame-slice) / var(--frame-width) round;
  border-radius:0; background-clip:border-box; padding:12px 34px 34px;
}
.panel,.replace{color:var(--black)}
.has-topper{position:relative; margin-top:24px}
.topper{position:absolute; left:50%; bottom:calc(100% - 13px); transform:translateX(-50%); max-width:100%; height:auto; pointer-events:none; filter:drop-shadow(0 0 5px rgba(255,255,255,.55))}
.panel,.replace{
  border-width:7px;
  border-style:solid;
  border-image:url("scallop.png") 8 fill round;
  border-radius:0;
  background:var(--white);
  background-clip:padding-box;
}
.replace{position:relative}
.bow{position:absolute; top:-6px; right:-8px; width:64px; height:auto; transform:translate(25%,-35%) rotate(30deg); pointer-events:none}
.shell,.foot,#dialogs{zoom:var(--ui-scale)}

.topper,.bow,.divider{
  animation-name:floating;
  animation-duration:3s;
  animation-iteration-count:infinite;
  animation-timing-function:ease-in-out;
}
@keyframes floating{
  0%   {translate:0 0}
  50%  {translate:0 2px}
  100% {translate:0 0}
}
</style>
</head>
<body>
<div class="shell">
<main class="app">
  <h1>lue's ui modder <span class="heart">♡</span></h1>

  <section class="panel has-topper" aria-label="Roblox install type">
    <img class="topper" src="bow.png" alt="">
    <div class="modes">
      <button class="mode" id="m-vanilla" aria-pressed="true">vanilla roblox</button>
      <button class="mode" id="m-bloxstrap" aria-pressed="false">bloxstrap</button>
    </div>
  </section>

  <section class="panel main" aria-label="Image and targets">
    <div class="side">
      <h2>what would you like to replace?</h2>
      <img class="divider" src="div_lue.webp" alt="">
      <div class="targets" id="targets"></div>
    </div>
  </section>

  <button class="replace" id="replace">replace texture<img class="bow" src="bow.gif" alt=""></button>
</main>
</div>
<footer class="foot">discord.gg/GmQ9HMy2ZE</footer>

<div id="dialogs"></div>

<script>
const TARGETS = [
  { id:'emote',  label:'emote',       title:'emote image', desc:'png file' },
  { id:'player', label:'player list', title:'player list image', desc:'png file' },
  { id:'cursor', label:'cursor',      title:'cursor images', desc:'.zip file' }
];

const $ = s => document.querySelector(s);

const targetsEl = $('#targets');
const dialogsEl = $('#dialogs');

TARGETS.forEach(t => {
  const row = document.createElement('div');
  row.className = 'row';
  row.style.cursor = 'pointer';
  row.innerHTML = `
    <span class="box" role="checkbox" tabindex="0" aria-checked="false" aria-label="Replace ${t.label}">
      <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 21s-7.5-4.6-9.6-9.2C.9 8.2 3 4.5 6.6 4.5c2 0 3.7 1.1 5.4 3 1.7-1.9 3.4-3 5.4-3 3.6 0 5.7 3.7 4.2 7.3C19.5 16.4 12 21 12 21z"/></svg>
    </span>
    <span class="lbl">${t.label}</span>`;
  targetsEl.append(row);
  const box = row.querySelector('.box');

  const toggle = () => box.setAttribute('aria-checked', box.getAttribute('aria-checked') !== 'true');
  box.addEventListener('click', e => { e.stopPropagation(); toggle(); });
  box.addEventListener('keydown', e => { if(e.key===' '||e.key==='Enter'){ e.preventDefault(); e.stopPropagation(); toggle(); } });

  const dlg = document.createElement('dialog');
  dlg.id = 'dlg-' + t.id;
  dlg.setAttribute('aria-label', t.title);
  dlg.innerHTML = `
    <div class="dlg-head"><h3>${t.title} ♡</h3><button class="x" aria-label="Close">×</button></div>
    <button class="drop" id="upload-${t.id}"><span class="hint">upload image</span><span class="desc">${t.desc}</span></button>
    <div class="dlg-actions">
      <button class="clear">remove</button>
      <button class="primary save">save</button>
    </div>`;
  dialogsEl.append(dlg);

  row.addEventListener('click', () => dlg.showModal());
  dlg.querySelector('.x').addEventListener('click', () => dlg.close());
  dlg.addEventListener('click', e => { if(e.target === dlg) dlg.close(); });
  dlg.querySelector('.save').addEventListener('click', () => dlg.close());
});

const modes = ['#m-vanilla','#m-bloxstrap'].map($);
modes.forEach(b => b.addEventListener('click', () => modes.forEach(m => m.setAttribute('aria-pressed', m === b))));

</script>

</body>
</html>)HTML";
static const size_t kIndexHtmlSize = sizeof(kIndexHtml) - 1;