from Cryptodome.Cipher import AES
import binascii

import time
import socket
import os
from binascii import b2a_hex
from binascii import a2b_hex

host = "192.168.10.102"
port = 7000
key = bytes.fromhex('603deb1015ca71be2b73aef0857d7781')
nonce = bytes.fromhex('000000000000000000000000')


def encrypt(text, key):
    mode = AES.MODE_GCM
    cryptos = AES.new(key, mode,nonce=nonce)
    cipher_text,tag = cryptos.encrypt_and_digest(str.encode(text))
    return tag+cipher_text

def decrypt(text, key):
    mode = AES.MODE_GCM
    cryptor = AES.new(key, mode,nonce=nonce)
    plain_text = cryptor.decrypt(text)
    return plain_text


s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((host,port))
while True:
	request=input('SEND:\n')
	print('\n')
	send_text=encrypt(request,key)
	s.send(send_text)
	if request=='quit':
		break
	print('RECV:')
	enc_result=s.recv(1024)
	result=decrypt(enc_result[16:],key)
	print(result.decode(),'\n')
s.close()