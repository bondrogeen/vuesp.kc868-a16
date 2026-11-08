import { defineConfig, loadEnv } from 'vite';
import vue from '@vitejs/plugin-vue';
import tailwindcss from '@tailwindcss/vite';
import { fileURLToPath, URL } from 'node:url';
import VueRouter from 'unplugin-vue-router/vite';
import { compression } from 'vite-plugin-compression2';
import pkg from '../vue/package.json';

import Icons from 'unplugin-icons/vite';
import IconsResolver from 'unplugin-icons/resolver';
import Components from 'unplugin-vue-components/vite';

const path = fileURLToPath(new URL('./src', import.meta.url));

type PackageMeta = typeof pkg & {
  author?: string;
  repository?: unknown;
  homepage?: string;
  bugs?: unknown;
};

// vuesp-create removes the template repository links from package.json.
const pkgMeta = pkg as PackageMeta;
const { name, version, author, description, repository, license, homepage, bugs } = pkgMeta;
const info = { name, version, author, description, repository, license, homepage, bugs };

// https://vite.dev/config/
export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), 'VITE_');
  const authLogin = env.VITE_AUTH_LOGIN;
  const authPassword = env.VITE_AUTH_PASSWORD;
  const ip = env.VITE_IP || '127.0.0.1';
  const url = authLogin && authPassword ? `${authLogin}:${authPassword}@${ip}` : ip;

  return {
    build: {
      outDir: env.VITE_OUTPUT_DIR || '../data/www/',
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
        deleteOriginalAssets: true,
        include: /\.(js|css|svg|ttf|woff|eot|json)$/i,
        threshold: 0,
      }),
      Icons({ autoInstall: true }),
      Components({ resolvers: [IconsResolver({ prefix: 'icon', enabledCollections: ['ri'] })] }),
    ],
    resolve: {
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
          target: `ws://${url}`,
          ws: true,
        },
        '/fs': {
          target: `http://${url}`,
        },
        '/cmd': {
          target: `http://${url}`,
        },
        '/update': {
          target: `http://${url}`,
        },
      },
    },
    define: {
      __APP__: info,
    },
  };
});
