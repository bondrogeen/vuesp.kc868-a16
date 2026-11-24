import { defineConfig, loadEnv } from 'vite';
import vue from '@vitejs/plugin-vue';
import tailwindcss from '@tailwindcss/vite';
import { execFileSync } from 'node:child_process';
import { fileURLToPath, URL } from 'node:url';
import { Buffer } from 'node:buffer';
import { readFileSync } from 'node:fs';
import VueRouter from 'unplugin-vue-router/vite';
import { compression } from 'vite-plugin-compression2';
import pkg from '../vue/package.json';

import Components from 'unplugin-vue-components/vite';

const path = fileURLToPath(new URL('./src', import.meta.url));

type PackageMeta = typeof pkg & {
  author?: string;
  repository?: unknown;
  homepage?: string;
  bugs?: unknown;
};

const normalizeRepository = (value: string | undefined): string | undefined => {
  const remote = value?.trim();
  if (!remote) return undefined;
  if (remote.startsWith('git@')) {
    const match = /^git@([^:]+):(.+)$/.exec(remote);
    return match ? `https://${match[1]}/${match[2].replace(/\.git$/, '')}` : undefined;
  }
  if (remote.startsWith('ssh://')) {
    try {
      const url = new URL(remote);
      return `https://${url.hostname}${url.pathname.replace(/\.git$/, '')}`;
    } catch {
      return undefined;
    }
  }
  if (/^(https?|git):\/\//.test(remote)) {
    return remote.replace(/^git:\/\//, 'https://').replace(/\.git$/, '');
  }
  return undefined;
};

const gitRepository = (() => {
  try {
    return normalizeRepository(
      execFileSync('git', ['config', '--get', 'remote.origin.url'], {
        cwd: fileURLToPath(new URL('.', import.meta.url)),
        encoding: 'utf8',
        stdio: ['ignore', 'pipe', 'ignore'],
      })
    );
  } catch {
    return undefined;
  }
})();

const pkgMeta = pkg as PackageMeta;
const { name, version, author, description, repository, license, bugs } = pkgMeta;
const packageRepository = repository as { url?: string } | undefined;
const repositoryUrl = packageRepository?.url ?? gitRepository;
const info = {
  name,
  version,
  author,
  description,
  repository: repositoryUrl ? { type: 'git', url: repositoryUrl } : undefined,
  license,
  homepage: 'https://vuesp.ru',
  bugs,
};

const parseFirmwareVersion = (text: string): [number, number, number] => {
  const match = /firmware_version\s*=\s*'\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}'/.exec(text);
  return match ? [Number(match[1]), Number(match[2]), Number(match[3])] : [4, 0, 0];
};

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), 'VITE_');
  const demoData = mode === 'demo'
    ? {
        defaultJson: readFileSync('public/default.json', 'utf8'),
        dashboardJson: readFileSync('json/dashboard.json', 'utf8'),
        scriptsTxt: readFileSync('public/scripts.txt', 'utf8'),
        firmwareVersion: parseFirmwareVersion(readFileSync('../platformio.ini', 'utf8')),
      }
    : null;
  const authLogin = env.VITE_AUTH_LOGIN;
  const authPassword = env.VITE_AUTH_PASSWORD;
  const ip = env.VITE_IP;
  const url = authLogin && authPassword ? `${authLogin}:${authPassword}@${ip}` : ip;
  const authHeader = authLogin && authPassword ? `Basic ${Buffer.from(`${authLogin}:${authPassword}`).toString('base64')}` : undefined;
  console.log(url);

  return {
    base: mode === 'demo' ? '/demo-app/' : '/',
    build: {
      outDir: mode === 'demo' ? '../demo-dist' : env.VITE_OUTPUT_DIR ?? '../data/www/',
      emptyOutDir: true,
      cssCodeSplit: false,
      rollupOptions: {
        output: {
          inlineDynamicImports: true,
          entryFileNames: `assets/[name][hash].js`,
          chunkFileNames: `assets/[name][hash].js`,
          assetFileNames: `assets/[name][hash].[ext]`,
        },
      },
    },
    plugins: [
      VueRouter(),
      vue(),
      tailwindcss(),
      compression({
        filename: '[path][base].gz',
        algorithm: 'gzip',
        deleteOriginalAssets: mode !== 'demo',
        include: /\.(js|css|svg|ttf|woff|eot|json)$/i,
        threshold: 0,
      }),
      Components(),
    ],
    resolve: {
      dedupe: ['vue', 'vue-router', 'pinia'],
      extensions: ['.mjs', '.js', '.ts', '.jsx', '.tsx', '.json', '.vue'],
      alias: {
        '@': path,
      },
    },
    server: {
      allowedHosts: ['test.vuesp.ru'],
      host: '0.0.0.0',
      port: 3000,
      proxy: {
        '/esp': {
          target: `ws://${ip}`,
          ws: true,
          changeOrigin: true,
          headers: authHeader ? { Authorization: authHeader } : undefined,
        },
        '/fs': {
          target: `http://${ip}`,
          changeOrigin: true,
          headers: authHeader ? { Authorization: authHeader } : undefined,
        },
        '/cmd': {
          target: `http://${ip}`,
          changeOrigin: true,
          headers: authHeader ? { Authorization: authHeader } : undefined,
        },
        '/update': {
          target: `http://${ip}`,
          changeOrigin: true,
          headers: authHeader ? { Authorization: authHeader } : undefined,
        },
      },
    },
    define: {
      __APP__: info,
      __DEMO_DATA__: JSON.stringify(demoData),
      __DEMO__: mode === 'demo',
    },
  };
});
