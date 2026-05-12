export default async (req) => {
  const urlString = new URL(req.url).searchParams.get('url');
  if (!urlString) {
    return new Response('Missing ?url=', { status: 400 });
  }

  try {
    const headers = {
      'User-Agent': 'OPX-Flasher/1.0',
    };
    
    // Pass through Accept header if present
    if (req.headers.get('accept')) {
      headers['Accept'] = req.headers.get('accept');
    }

    const resp = await fetch(urlString, {
      redirect: 'follow',
      headers: headers,
    });

    if (!resp.ok) {
      return new Response('Upstream error: ' + resp.status, { 
        status: resp.status,
        headers: { 'Access-Control-Allow-Origin': '*' } 
      });
    }

    const data = await resp.arrayBuffer();
    const filename = urlString.split('/').pop() || 'firmware.bin';

    return new Response(data, {
      status: 200,
      headers: {
        'Content-Type': resp.headers.get('content-type') || 'application/octet-stream',
        'Content-Disposition': 'attachment; filename="' + filename + '"',
        'Access-Control-Allow-Origin': '*',
        'Access-Control-Allow-Methods': 'GET, OPTIONS',
        'Access-Control-Allow-Headers': '*',
        'Cache-Control': 'public, max-age=3600'
      },
    });
  } catch (e) {
    return new Response('Proxy error: ' + e.message, { 
      status: 502,
      headers: { 'Access-Control-Allow-Origin': '*' }
    });
  }
};

export const config = {
  path: '/api/proxy',
};
