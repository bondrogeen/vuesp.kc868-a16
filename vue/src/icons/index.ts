import { ICONS, type IIconData } from 'vuesp-components/components';

interface ILocalIcon {
	name: string;
	data: IIconData;
}

const files = import.meta.glob('./*.svg', {
	eager: true,
	query: '?raw',
	import: 'default',
}) as Record<string, string>;

const parseSvg = (raw: string, name: string): ILocalIcon | null => {
	const match = raw.match(/<svg\b([^>]*)>([\s\S]*?)<\/svg>/i);
	if (!match) {
		console.warn(`[icons] ${name}: не удалось прочитать SVG`);
		return null;
	}

	const viewBox = /viewBox=["']([^"']+)["']/i.exec(match[1])?.[1] ?? '0 0 24 24';
	const isStroke = /\bstroke=/i.test(match[1]) && /fill=["']none["']/i.test(match[1]);
	const strokeWidth = Number(/stroke-width=["']([^"']+)["']/i.exec(match[1])?.[1] ?? 1.2);

	return { name, data: { body: match[2], viewBox, ...(isStroke ? { mode: 'stroke', strokeWidth } : {}) } };
};

export const localIcons: Record<string, IIconData> = {};
export const localIconNames: string[] = [];

for (const [path, raw] of Object.entries(files)) {
	const name = path.replace(/^\.\//, '').replace(/\.svg$/i, '').toLowerCase();

	if (!/^[a-z0-9-]+$/.test(name)) {
		console.warn(`[icons] ${path}: имя должно быть в kebab-case`);
		continue;
	}

	if (name in ICONS) {
		console.warn(`[icons] ${path}: имя «${name}» занято базовой иконкой`);
		continue;
	}

	const icon = parseSvg(raw, name);
	if (!icon) continue;

	localIcons[name] = icon.data;
	localIconNames.push(name);
}
