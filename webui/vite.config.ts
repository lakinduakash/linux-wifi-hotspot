import { defineConfig, loadEnv } from 'vite';
import { fileURLToPath, URL } from 'node:url';
import { svelte } from '@sveltejs/vite-plugin-svelte';
import svelteConfig from './svelte.config.js';

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '');
  const port = env.VITE_PORT ? Number(env.VITE_PORT) : 5173;

  return {
    plugins: [
      svelte({
        ...svelteConfig,
      }),
    ],
    server: {
      host: '127.0.0.1',
      port,
    },
    resolve: {
      alias: {
        $lib: fileURLToPath(new URL('./src/lib', import.meta.url)),
      },
    },
  };
});
