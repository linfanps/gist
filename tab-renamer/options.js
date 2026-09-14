const list = document.getElementById('list');

async function getRules() {
  const { rules = [] } = await chrome.storage.sync.get('rules');
  return rules;
}

async function setRules(rules) {
  await chrome.storage.sync.set({ rules });
}

function render(rules) {
  list.replaceChildren();
  document.getElementById('count').textContent = rules.length ? `共 ${rules.length} 条` : '';

  if (!rules.length) {
    const empty = document.createElement('div');
    empty.className = 'empty';
    empty.textContent = '还没有规则。在任意页面点扩展图标即可添加。';
    list.append(empty);
    return;
  }

  const table = document.createElement('table');
  const thead = document.createElement('thead');
  const hr = document.createElement('tr');
  for (const [text, width] of [['范围', '64px'], ['匹配目标', ''], ['自定义标题', '200px'], ['', '48px']]) {
    const th = document.createElement('th');
    th.textContent = text;
    if (width) th.style.width = width;
    hr.append(th);
  }
  thead.append(hr);
  table.append(thead);

  const tbody = document.createElement('tbody');
  // 网站级规则排在前面，同级按标题排序，方便查找
  const sorted = [...rules].sort(
    (a, b) => a.mode.localeCompare(b.mode) || a.title.localeCompare(b.title)
  );

  for (const rule of sorted) {
    const tr = document.createElement('tr');

    const tdMode = document.createElement('td');
    const tag = document.createElement('span');
    tag.className = 'tag';
    tag.textContent = rule.mode === 'host' ? '网站' : '页面';
    tdMode.append(tag);

    const tdTarget = document.createElement('td');
    tdTarget.className = 'target';
    tdTarget.textContent = rule.value;

    const tdTitle = document.createElement('td');
    const input = document.createElement('input');
    input.type = 'text';
    input.value = rule.title;
    input.addEventListener('change', async () => {
      const title = input.value.trim();
      const current = await getRules();
      if (!title) {
        await setRules(current.filter((r) => r.id !== rule.id));
        return;
      }
      await setRules(current.map((r) => (r.id === rule.id ? { ...r, title } : r)));
    });
    tdTitle.append(input);

    const tdDel = document.createElement('td');
    const del = document.createElement('button');
    del.className = 'del';
    del.textContent = '删除';
    del.addEventListener('click', async () => {
      const current = await getRules();
      await setRules(current.filter((r) => r.id !== rule.id));
    });
    tdDel.append(del);

    tr.append(tdMode, tdTarget, tdTitle, tdDel);
    tbody.append(tr);
  }

  table.append(tbody);
  list.append(table);
}

// ---------- 导入 / 导出 ----------

const EXPORT_VERSION = 1;

function newId() {
  return `${Date.now()}-${Math.random().toString(36).slice(2, 8)}`;
}

function status(text, ok = true) {
  const el = document.getElementById('status');
  el.textContent = text;
  el.className = `status ${ok ? 'ok' : 'bad'}`;
  el.hidden = false;
}

async function exportRules() {
  const rules = await getRules();
  if (!rules.length) return status('没有规则可导出。', false);

  const payload = {
    format: 'tab-renamer-rules',
    version: EXPORT_VERSION,
    exportedAt: new Date().toISOString(),
    rules,
  };
  const blob = new Blob([JSON.stringify(payload, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `tab-renamer-rules-${new Date().toISOString().slice(0, 10)}.json`;
  a.click();
  URL.revokeObjectURL(url);

  status(`已导出 ${rules.length} 条规则。`);
}

// 接受两种形式：导出文件 { rules: [...] }，或者直接一个规则数组
function parseRules(text) {
  const data = JSON.parse(text);
  const raw = Array.isArray(data) ? data : data?.rules;
  if (!Array.isArray(raw)) throw new Error('文件里找不到规则数组');

  const valid = [];
  let dropped = 0;
  for (const r of raw) {
    const mode = r?.mode;
    const value = typeof r?.value === 'string' ? r.value.trim() : '';
    const title = typeof r?.title === 'string' ? r.title.trim() : '';
    if ((mode === 'url' || mode === 'host') && value && title) {
      valid.push({ id: typeof r.id === 'string' ? r.id : newId(), mode, value, title });
    } else {
      dropped++;
    }
  }
  return { valid, dropped };
}

async function importRules(file) {
  let parsed;
  try {
    parsed = parseRules(await file.text());
  } catch (e) {
    return status(`导入失败：${e.message}`, false);
  }
  if (!parsed.valid.length) return status('文件里没有有效规则。', false);

  // 合并进现有规则：同一个 (mode, value) 只留一条，以导入的为准
  const current = await getRules();
  const merged = new Map(current.map((r) => [`${r.mode}\x00${r.value}`, r]));
  let added = 0;
  let updated = 0;
  for (const r of parsed.valid) {
    const key = `${r.mode}\x00${r.value}`;
    if (merged.has(key)) updated++;
    else added++;
    merged.set(key, r);
  }
  await setRules([...merged.values()]);

  const parts = [`新增 ${added} 条`];
  if (updated) parts.push(`覆盖 ${updated} 条`);
  if (parsed.dropped) parts.push(`忽略 ${parsed.dropped} 条无效数据`);
  status(`导入完成：${parts.join('，')}。`);
}

async function clearAll() {
  const rules = await getRules();
  if (!rules.length) return status('本来就没有规则。', false);
  if (!confirm(`确定要删除全部 ${rules.length} 条规则吗？此操作无法撤销。`)) return;
  await setRules([]);
  status('已清空全部规则。');
}

const fileInput = document.getElementById('file');
document.getElementById('export').addEventListener('click', exportRules);
document.getElementById('import').addEventListener('click', () => fileInput.click());
document.getElementById('clear').addEventListener('click', clearAll);
fileInput.addEventListener('change', () => {
  const file = fileInput.files?.[0];
  if (file) importRules(file);
  fileInput.value = ''; // 允许连续导入同一个文件
});

// 存储变化时重绘，多个窗口同时打开也能保持一致
chrome.storage.onChanged.addListener((changes, area) => {
  if (area === 'sync' && changes.rules) render(changes.rules.newValue || []);
});

getRules().then(render);
