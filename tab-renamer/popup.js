const $ = (id) => document.getElementById(id);

let info = null; // content script 回报的当前页面信息

async function init() {
  const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
  if (!tab?.id) return fail();

  try {
    info = await chrome.tabs.sendMessage(tab.id, { type: 'getInfo' });
  } catch {
    return fail();
  }
  if (!info?.ok) return fail();

  $('main').hidden = false;
  $('orig').textContent = info.originalTitle || '(无标题)';
  $('urlVal').textContent = info.normalizedUrl;
  $('hostVal').textContent = info.host;

  // 如果当前页面已有规则，回填进来
  const { rules = [] } = await chrome.storage.sync.get('rules');
  const urlRule = rules.find((r) => r.mode === 'url' && r.value === info.normalizedUrl);
  const hostRule = rules.find((r) => r.mode === 'host' && r.value === info.host);
  const hit = urlRule || hostRule;
  if (hit) {
    $('title').value = hit.title;
    document.querySelector(`input[name="mode"][value="${hit.mode}"]`).checked = true;
  }

  $('title').focus();
  $('title').select();
}

function fail() {
  $('err').hidden = false;
}

function selectedMode() {
  return document.querySelector('input[name="mode"]:checked').value;
}

async function save() {
  const title = $('title').value.trim();
  const mode = selectedMode();
  const value = mode === 'url' ? info.normalizedUrl : info.host;

  const { rules = [] } = await chrome.storage.sync.get('rules');
  // 同一个目标只保留一条规则
  const next = rules.filter((r) => !(r.mode === mode && r.value === value));
  if (title) {
    next.push({
      id: `${Date.now()}-${Math.random().toString(36).slice(2, 8)}`,
      mode,
      value,
      title,
    });
  }
  await chrome.storage.sync.set({ rules: next });
  window.close();
}

// 删掉命中当前页面的所有规则（不管是页面级还是网站级）
async function reset() {
  const { rules = [] } = await chrome.storage.sync.get('rules');
  const next = rules.filter(
    (r) => !(r.mode === 'url' && r.value === info.normalizedUrl) && !(r.mode === 'host' && r.value === info.host)
  );
  await chrome.storage.sync.set({ rules: next });

  // 规则没了，content script 不会主动把标题改回去，这里显式还原一次
  const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
  if (tab?.id) await chrome.tabs.reload(tab.id);
  window.close();
}

$('save').addEventListener('click', save);
$('reset').addEventListener('click', reset);
$('title').addEventListener('keydown', (e) => {
  if (e.key === 'Enter') save();
});
$('opts').addEventListener('click', (e) => {
  e.preventDefault();
  chrome.runtime.openOptionsPage();
});

init();
