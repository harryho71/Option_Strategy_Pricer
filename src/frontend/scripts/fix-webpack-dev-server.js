/**
 * Replaces the deprecated onBeforeSetupMiddleware / onAfterSetupMiddleware
 * options in CRA's webpackDevServer.config.js with the new setupMiddlewares API.
 * Run automatically via the "postinstall" npm script.
 */
const fs = require("fs");
const path = require("path");

const configPath = path.join(
  __dirname,
  "..",
  "node_modules",
  "react-scripts",
  "config",
  "webpackDevServer.config.js"
);

if (!fs.existsSync(configPath)) {
  console.log("[fix-webpack-dev-server] File not found, skipping.");
  process.exit(0);
}

let content = fs.readFileSync(configPath, "utf8");

if (content.includes("setupMiddlewares")) {
  console.log("[fix-webpack-dev-server] Already patched, skipping.");
  process.exit(0);
}

const deprecated = `    onBeforeSetupMiddleware(devServer) {
      // Keep \`evalSourceMapMiddleware\`
      // middlewares before \`redirectServedPath\` otherwise will not have any effect
      // This lets us fetch source contents from webpack for the error overlay
      devServer.app.use(evalSourceMapMiddleware(devServer));

      if (fs.existsSync(paths.proxySetup)) {
        // This registers user provided middleware for proxy reasons
        require(paths.proxySetup)(devServer.app);
      }
    },
    onAfterSetupMiddleware(devServer) {
      // Redirect to \`PUBLIC_URL\` or \`homepage\` from \`package.json\` if url not match
      devServer.app.use(redirectServedPath(paths.publicUrlOrPath));

      // This service worker file is effectively a 'no-op' that will reset any
      // previous service worker registered for the same host:port combination.
      // We do this in development to avoid hitting the production cache if
      // it used the same host and port.
      // https://github.com/facebook/create-react-app/issues/2272#issuecomment-302832432
      devServer.app.use(noopServiceWorkerMiddleware(paths.publicUrlOrPath));
    },`;

const replacement = `    setupMiddlewares(middlewares, devServer) {
      // Keep \`evalSourceMapMiddleware\`
      // middlewares before \`redirectServedPath\` otherwise will not have any effect
      // This lets us fetch source contents from webpack for the error overlay
      middlewares.unshift(evalSourceMapMiddleware(devServer));

      if (fs.existsSync(paths.proxySetup)) {
        // This registers user provided middleware for proxy reasons
        require(paths.proxySetup)(devServer.app);
      }

      // Redirect to \`PUBLIC_URL\` or \`homepage\` from \`package.json\` if url not match
      middlewares.push(redirectServedPath(paths.publicUrlOrPath));

      // This service worker file is effectively a 'no-op' that will reset any
      // previous service worker registered for the same host:port combination.
      // We do this in development to avoid hitting the production cache if
      // it used the same host and port.
      // https://github.com/facebook/create-react-app/issues/2272#issuecomment-302832432
      middlewares.push(noopServiceWorkerMiddleware(paths.publicUrlOrPath));

      return middlewares;
    },`;

if (!content.includes(deprecated)) {
  console.log("[fix-webpack-dev-server] Pattern not found — may already be patched or version changed.");
  process.exit(0);
}

content = content.replace(deprecated, replacement);
fs.writeFileSync(configPath, content, "utf8");
console.log("[fix-webpack-dev-server] Patched webpackDevServer.config.js successfully.");
