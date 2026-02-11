/**
 * AES-256-GCM encryption via WebAssembly (Emscripten-compiled).
 * Loads aes_gcm_encrypt.wasm from /wasm/ and exposes encrypt API.
 */

export interface EncryptedPayload {
  nonce: string;
  ct: string;
  tag: string;
}

let moduleInstance: AesGcmModule | null = null;

export interface AesGcmModule {
  ccall: (
    ident: string,
    returnType: string,
    argTypes: string[],
    args?: unknown[]
  ) => number;
  UTF8ToString: (ptr: number) => string;
}

declare global {
  interface Window {
    AesGcmEncrypt?: () => Promise<AesGcmModule>;
  }
}

/**
 * Load the WASM module. Call once before using encrypt.
 * Expects aes_gcm_encrypt.js and aes_gcm_encrypt.wasm in /wasm/
 */
export async function loadEncryptionModule(): Promise<AesGcmModule> {
  if (moduleInstance) return moduleInstance;

  if (!window.AesGcmEncrypt) {
    await new Promise<void>((resolve, reject) => {
      const s = document.createElement('script');
      s.src = '/wasm/aes_gcm_encrypt.js';
      s.async = true;
      s.onload = () => resolve();
      s.onerror = () => reject(new Error('Failed to load aes_gcm_encrypt.js'));
      document.head.appendChild(s);
    });
  }

  const createModule = window.AesGcmEncrypt;
  if (!createModule) throw new Error('AesGcmEncrypt not found');

  const module = await createModule();
  moduleInstance = module;
  return moduleInstance;
}

/**
 * Generate a random 12-byte nonce as hex string.
 */
function randomNonceHex(): string {
  const bytes = new Uint8Array(12);
  crypto.getRandomValues(bytes);
  return Array.from(bytes)
    .map((b) => b.toString(16).padStart(2, '0'))
    .join('');
}

/**
 * Encrypt plaintext with AES-256-GCM.
 * @param plaintext - UTF-8 string (e.g. JSON.stringify(msg))
 * @param keyHex - 32-byte key as 64 hex chars
 * @param nonceHex - 12-byte nonce as 24 hex chars (optional; generated if not provided)
 * @returns Encrypted payload { nonce, ct, tag } or null on failure
 */
export function encrypt(
  plaintext: string,
  keyHex: string,
  nonceHex?: string
): EncryptedPayload | null {
  if (!moduleInstance) {
    console.error('Encryption module not loaded. Call loadEncryptionModule() first.');
    return null;
  }

  const nonce = nonceHex ?? randomNonceHex();

  const resultPtr = moduleInstance.ccall(
    'encrypt_aes_gcm_json',
    'number',
    ['string', 'string', 'string'],
    [keyHex, nonce, plaintext]
  );

  if (!resultPtr) return null;

  try {
    const jsonStr = moduleInstance.UTF8ToString(resultPtr);
    (moduleInstance as { ccall: AesGcmModule['ccall'] }).ccall('free_string', null, ['number'], [resultPtr]);
    const parsed = JSON.parse(jsonStr) as EncryptedPayload;
    return parsed;
  } catch {
    (moduleInstance as { ccall: AesGcmModule['ccall'] }).ccall('free_string', null, ['number'], [resultPtr]);
    return null;
  }
}

/**
 * Encrypt a JSON object and return a string suitable for WebSocket send.
 * Output format: JSON string of { nonce, ct, tag } that the robot can decrypt.
 */
export function encryptJson(
  obj: object,
  keyHex: string
): string | null {
  const plaintext = JSON.stringify(obj);
  const encrypted = encrypt(plaintext, keyHex);
  if (!encrypted) return null;
  return JSON.stringify(encrypted);
}
