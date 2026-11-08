function parseMarkdown(md) {
  const lines = md.split("\n");
  let html = "";
  let meta = { version: "—", lastUpdate: "—", lang: "ru" };
  let inCode = false,
    codeContent = "",
    inTable = false,
    tableRows = [],
    tableHeaders = [],
    inWarning = false,
    warningContent = "",
    sectionLevel = 0;
  for (let i = 0; i < lines.length; i++) {
    let line = lines[i];
    if (line.startsWith("@meta:")) {
      const p = line.substring(6).split("=");
      if (p.length === 2) meta[p[0].trim()] = p[1].trim();
      continue;
    }
    if (line.startsWith("# ") && !line.startsWith("##")) {
      html += `<h1>${esc(line.substring(2))}</h1>`;
      continue;
    }
    if (line.startsWith("## ") && !line.startsWith("###")) {
      if (sectionLevel >= 2) html += `</div>`;
      html += `<div class="section"><h2>${esc(line.substring(3))}</h2>`;
      sectionLevel = 2;
      continue;
    }
    if (line.startsWith("### ")) {
      html += `<h3>${esc(line.substring(4))}</h3>`;
      continue;
    }
    if (line.trim() === "@code") {
      inCode = true;
      codeContent = "";
      continue;
    }
    if (inCode) {
      if (line.trim() === "@end") {
        inCode = false;
        html += `<pre class="code-block">${esc(codeContent)}</pre>`;
        continue;
      }
      codeContent += line + "\n";
      continue;
    }
    if (line.trim() === "@table") {
      inTable = true;
      tableRows = [];
      tableHeaders = [];
      continue;
    }
    if (inTable) {
      if (line.trim() === "@end") {
        inTable = false;
        html += renderTable(tableHeaders, tableRows);
        continue;
      }
      if (line.startsWith("|")) {
        const c = line
          .split("|")
          .filter((x) => x.trim())
          .map((x) => x.trim());
        if (c.every((x) => /^[-:]+$/.test(x))) continue;
        if (tableHeaders.length === 0) tableHeaders = c;
        else tableRows.push(c);
      }
      continue;
    }
    if (line.trim() === "@warning") {
      inWarning = true;
      warningContent = "";
      continue;
    }
    if (inWarning) {
      if (line.trim() === "@end") {
        inWarning = false;
        html += `<div class="warning">${esc(warningContent)}</div>`;
        continue;
      }
      warningContent += line + "\n";
      continue;
    }
    if (line.startsWith("@note:")) {
      html += `<div class="note">${esc(line.substring(6))}</div>`;
      continue;
    }
    if (line.trim() && !line.startsWith("---")) html += `<p>${esc(line)}</p>`;
  }
  if (sectionLevel >= 2) html += `</div>`;
  return { html, meta };
}
function esc(t) {
  const d = document.createElement("div");
  d.textContent = t;
  return d.innerHTML;
}
function renderTable(h, r) {
  if (!h.length || !r.length) return "";
  let html = `<div class="table-wrap"><table class="table-docs"><thead><tr>`;
  h.forEach((x) => (html += `<th>${esc(x)}</th>`));
  html += `</tr></thead><tbody>`;
  r.forEach((row) => {
    html += `<tr>`;
    row.forEach((c) => (html += `<td>${esc(c)}</td>`));
    html += `</tr>`;
  });
  html += `</tbody></table></div>`;
  return html;
}
