const http = require('http');
const fs = require('fs');
const path = require('path');
const { URL } = require('url');
const zlib = require('zlib');

const rootDir = path.resolve(process.argv[2] || path.join(__dirname, '..', '..', 'out', 'build', 'web-release', 'GameRuntime'));
const port = Number.parseInt(process.argv[3] || '8000', 10);

const mimeTypes = {
    '.css': 'text/css; charset=utf-8',
    '.data': 'application/octet-stream',
    '.html': 'text/html; charset=utf-8',
    '.js': 'text/javascript; charset=utf-8',
    '.json': 'application/json; charset=utf-8',
    '.map': 'application/json; charset=utf-8',
    '.png': 'image/png',
    '.svg': 'image/svg+xml',
    '.txt': 'text/plain; charset=utf-8',
    '.wasm': 'application/wasm'
};

// Small text files that can be compressed on-the-fly without much overhead
const compressibleExtensions = new Set([
    '.css',
    '.html',
    '.json',
    '.map',
    '.svg',
    '.txt'
]);

function setCommonHeaders(response)
{
    response.setHeader('Cache-Control', 'no-cache, no-store, must-revalidate');
    response.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
    response.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
    response.setHeader('Cross-Origin-Resource-Policy', 'cross-origin');
    response.setHeader('Vary', 'Accept-Encoding');
}

function getContentEncoding(request, extension)
{
    if (!compressibleExtensions.has(extension))
        return null;

    const acceptEncoding = request.headers['accept-encoding'] || '';
    if (acceptEncoding.includes('br'))
        return 'br';
    if (acceptEncoding.includes('gzip'))
        return 'gzip';

    return null;
}

function tryGetDefaultFile(directory)
{
    const indexPath = path.join(directory, 'index.html');
    if (fs.existsSync(indexPath))
        return indexPath;

    const htmlFiles = fs.readdirSync(directory)
        .filter((entry) => entry.toLowerCase().endsWith('.html'))
        .sort();

    return htmlFiles.length > 0 ? path.join(directory, htmlFiles[0]) : null;
}

const server = http.createServer((request, response) =>
{
    setCommonHeaders(response);

    const requestUrl = new URL(request.url, `http://${request.headers.host || 'localhost'}`);
    const decodedPath = decodeURIComponent(requestUrl.pathname);
    const relativePath = decodedPath === '/' ? '.' : decodedPath.replace(/^\/+/, '');
    const resolvedPath = path.resolve(rootDir, relativePath);

    if (!resolvedPath.startsWith(rootDir))
    {
        response.writeHead(403, { 'Content-Type': 'text/plain; charset=utf-8' });
        response.end('Forbidden');
        return;
    }

    let filePath = resolvedPath;
    if (fs.existsSync(filePath) && fs.statSync(filePath).isDirectory())
    {
        const defaultFile = tryGetDefaultFile(filePath);
        if (!defaultFile)
        {
            response.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
            response.end('No HTML entry point found.');
            return;
        }
        filePath = defaultFile;
    }

    if (!fs.existsSync(filePath) || !fs.statSync(filePath).isFile())
    {
        response.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
        response.end('Not found');
        return;
    }

    const extension = path.extname(filePath).toLowerCase();
    const contentType = mimeTypes[extension] || 'application/octet-stream';
    const acceptEncoding = request.headers['accept-encoding'] || '';

    response.statusCode = 200;
    response.setHeader('Content-Type', contentType);

    // Try to serve pre-compressed files first (much faster than on-the-fly compression)
    const brPath = filePath + '.br';
    const gzPath = filePath + '.gz';

    if (acceptEncoding.includes('br') && fs.existsSync(brPath))
    {
        // Serve pre-compressed Brotli file
        response.setHeader('Content-Encoding', 'br');
        fs.createReadStream(brPath).pipe(response);
    }
    else if (acceptEncoding.includes('gzip') && fs.existsSync(gzPath))
    {
        // Serve pre-compressed gzip file
        response.setHeader('Content-Encoding', 'gzip');
        fs.createReadStream(gzPath).pipe(response);
    }
    else
    {
        // Fall back to on-the-fly compression for compressible types
        const contentEncoding = getContentEncoding(request, extension);
        let stream = fs.createReadStream(filePath);

        if (contentEncoding === 'br')
        {
            response.setHeader('Content-Encoding', 'br');
            stream = stream.pipe(zlib.createBrotliCompress());
        }
        else if (contentEncoding === 'gzip')
        {
            response.setHeader('Content-Encoding', 'gzip');
            stream = stream.pipe(zlib.createGzip());
        }

        stream.pipe(response);
    }
});

server.listen(port, () =>
{
    console.log(`Serving ${rootDir}`);
    console.log(`Open http://localhost:${port}/`);
    console.log('COOP/COEP headers enabled for Emscripten pthread/shared-memory builds.');
});
