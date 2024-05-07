
./bin/cargon:     file format elf64-x86-64


Disassembly of section .init:

0000000000001000 <_init>:
    1000:	f3 0f 1e fa          	endbr64
    1004:	48 83 ec 08          	sub    $0x8,%rsp
    1008:	48 8b 05 c1 2f 00 00 	mov    0x2fc1(%rip),%rax        # 3fd0 <__gmon_start__@Base>
    100f:	48 85 c0             	test   %rax,%rax
    1012:	74 02                	je     1016 <_init+0x16>
    1014:	ff d0                	call   *%rax
    1016:	48 83 c4 08          	add    $0x8,%rsp
    101a:	c3                   	ret

Disassembly of section .plt:

0000000000001020 <free@plt-0x10>:
    1020:	ff 35 ca 2f 00 00    	push   0x2fca(%rip)        # 3ff0 <_GLOBAL_OFFSET_TABLE_+0x8>
    1026:	ff 25 cc 2f 00 00    	jmp    *0x2fcc(%rip)        # 3ff8 <_GLOBAL_OFFSET_TABLE_+0x10>
    102c:	0f 1f 40 00          	nopl   0x0(%rax)

0000000000001030 <free@plt>:
    1030:	ff 25 ca 2f 00 00    	jmp    *0x2fca(%rip)        # 4000 <free@GLIBC_2.2.5>
    1036:	68 00 00 00 00       	push   $0x0
    103b:	e9 e0 ff ff ff       	jmp    1020 <_init+0x20>

0000000000001040 <puts@plt>:
    1040:	ff 25 c2 2f 00 00    	jmp    *0x2fc2(%rip)        # 4008 <puts@GLIBC_2.2.5>
    1046:	68 01 00 00 00       	push   $0x1
    104b:	e9 d0 ff ff ff       	jmp    1020 <_init+0x20>

0000000000001050 <strlen@plt>:
    1050:	ff 25 ba 2f 00 00    	jmp    *0x2fba(%rip)        # 4010 <strlen@GLIBC_2.2.5>
    1056:	68 02 00 00 00       	push   $0x2
    105b:	e9 c0 ff ff ff       	jmp    1020 <_init+0x20>

0000000000001060 <strchr@plt>:
    1060:	ff 25 b2 2f 00 00    	jmp    *0x2fb2(%rip)        # 4018 <strchr@GLIBC_2.2.5>
    1066:	68 03 00 00 00       	push   $0x3
    106b:	e9 b0 ff ff ff       	jmp    1020 <_init+0x20>

0000000000001070 <printf@plt>:
    1070:	ff 25 aa 2f 00 00    	jmp    *0x2faa(%rip)        # 4020 <printf@GLIBC_2.2.5>
    1076:	68 04 00 00 00       	push   $0x4
    107b:	e9 a0 ff ff ff       	jmp    1020 <_init+0x20>

0000000000001080 <memset@plt>:
    1080:	ff 25 a2 2f 00 00    	jmp    *0x2fa2(%rip)        # 4028 <memset@GLIBC_2.2.5>
    1086:	68 05 00 00 00       	push   $0x5
    108b:	e9 90 ff ff ff       	jmp    1020 <_init+0x20>

0000000000001090 <memcpy@plt>:
    1090:	ff 25 9a 2f 00 00    	jmp    *0x2f9a(%rip)        # 4030 <memcpy@GLIBC_2.14>
    1096:	68 06 00 00 00       	push   $0x6
    109b:	e9 80 ff ff ff       	jmp    1020 <_init+0x20>

00000000000010a0 <malloc@plt>:
    10a0:	ff 25 92 2f 00 00    	jmp    *0x2f92(%rip)        # 4038 <malloc@GLIBC_2.2.5>
    10a6:	68 07 00 00 00       	push   $0x7
    10ab:	e9 70 ff ff ff       	jmp    1020 <_init+0x20>

00000000000010b0 <realloc@plt>:
    10b0:	ff 25 8a 2f 00 00    	jmp    *0x2f8a(%rip)        # 4040 <realloc@GLIBC_2.2.5>
    10b6:	68 08 00 00 00       	push   $0x8
    10bb:	e9 60 ff ff ff       	jmp    1020 <_init+0x20>

00000000000010c0 <memmove@plt>:
    10c0:	ff 25 82 2f 00 00    	jmp    *0x2f82(%rip)        # 4048 <memmove@GLIBC_2.2.5>
    10c6:	68 09 00 00 00       	push   $0x9
    10cb:	e9 50 ff ff ff       	jmp    1020 <_init+0x20>

Disassembly of section .text:

00000000000010d0 <main>:
    10d0:	55                   	push   %rbp
    10d1:	bf 64 00 00 00       	mov    $0x64,%edi
    10d6:	53                   	push   %rbx
    10d7:	48 83 ec 08          	sub    $0x8,%rsp
    10db:	e8 c0 ff ff ff       	call   10a0 <malloc@plt>
    10e0:	48 85 c0             	test   %rax,%rax
    10e3:	0f 84 87 00 00 00    	je     1170 <main+0xa0>
    10e9:	66 0f 6f 05 7f 0f 00 	movdqa 0xf7f(%rip),%xmm0        # 2070 <_IO_stdin_used+0x70>
    10f0:	00 
    10f1:	48 89 c7             	mov    %rax,%rdi
    10f4:	48 89 c3             	mov    %rax,%rbx
    10f7:	0f 11 00             	movups %xmm0,(%rax)
    10fa:	66 0f 6f 05 7e 0f 00 	movdqa 0xf7e(%rip),%xmm0        # 2080 <_IO_stdin_used+0x80>
    1101:	00 
    1102:	0f 11 40 0c          	movups %xmm0,0xc(%rax)
    1106:	e8 45 03 00 00       	call   1450 <cloneString>
    110b:	48 89 c5             	mov    %rax,%rbp
    110e:	48 85 c0             	test   %rax,%rax
    1111:	74 4a                	je     115d <main+0x8d>
    1113:	48 8b 35 46 2f 00 00 	mov    0x2f46(%rip),%rsi        # 4060 <WHITE_SPACE>
    111a:	48 89 c7             	mov    %rax,%rdi
    111d:	e8 7e 03 00 00       	call   14a0 <stripString>
    1122:	48 89 de             	mov    %rbx,%rsi
    1125:	48 8d 3d 09 0f 00 00 	lea    0xf09(%rip),%rdi        # 2035 <_IO_stdin_used+0x35>
    112c:	31 c0                	xor    %eax,%eax
    112e:	e8 3d ff ff ff       	call   1070 <printf@plt>
    1133:	48 89 df             	mov    %rbx,%rdi
    1136:	e8 f5 fe ff ff       	call   1030 <free@plt>
    113b:	48 89 ee             	mov    %rbp,%rsi
    113e:	48 8d 3d 07 0f 00 00 	lea    0xf07(%rip),%rdi        # 204c <_IO_stdin_used+0x4c>
    1145:	31 c0                	xor    %eax,%eax
    1147:	e8 24 ff ff ff       	call   1070 <printf@plt>
    114c:	48 89 ef             	mov    %rbp,%rdi
    114f:	e8 dc fe ff ff       	call   1030 <free@plt>
    1154:	31 c0                	xor    %eax,%eax
    1156:	48 83 c4 08          	add    $0x8,%rsp
    115a:	5b                   	pop    %rbx
    115b:	5d                   	pop    %rbp
    115c:	c3                   	ret
    115d:	48 8d 3d ba 0e 00 00 	lea    0xeba(%rip),%rdi        # 201e <_IO_stdin_used+0x1e>
    1164:	e8 d7 fe ff ff       	call   1040 <puts@plt>
    1169:	b8 01 00 00 00       	mov    $0x1,%eax
    116e:	eb e6                	jmp    1156 <main+0x86>
    1170:	48 8d 3d 8d 0e 00 00 	lea    0xe8d(%rip),%rdi        # 2004 <_IO_stdin_used+0x4>
    1177:	e8 c4 fe ff ff       	call   1040 <puts@plt>
    117c:	eb eb                	jmp    1169 <main+0x99>
    117e:	66 90                	xchg   %ax,%ax

0000000000001180 <_start>:
    1180:	f3 0f 1e fa          	endbr64
    1184:	31 ed                	xor    %ebp,%ebp
    1186:	49 89 d1             	mov    %rdx,%r9
    1189:	5e                   	pop    %rsi
    118a:	48 89 e2             	mov    %rsp,%rdx
    118d:	48 83 e4 f0          	and    $0xfffffffffffffff0,%rsp
    1191:	50                   	push   %rax
    1192:	54                   	push   %rsp
    1193:	45 31 c0             	xor    %r8d,%r8d
    1196:	31 c9                	xor    %ecx,%ecx
    1198:	48 8d 3d 31 ff ff ff 	lea    -0xcf(%rip),%rdi        # 10d0 <main>
    119f:	ff 15 1b 2e 00 00    	call   *0x2e1b(%rip)        # 3fc0 <__libc_start_main@GLIBC_2.34>
    11a5:	f4                   	hlt
    11a6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
    11ad:	00 00 00 
    11b0:	48 8d 3d b1 2e 00 00 	lea    0x2eb1(%rip),%rdi        # 4068 <__TMC_END__>
    11b7:	48 8d 05 aa 2e 00 00 	lea    0x2eaa(%rip),%rax        # 4068 <__TMC_END__>
    11be:	48 39 f8             	cmp    %rdi,%rax
    11c1:	74 15                	je     11d8 <_start+0x58>
    11c3:	48 8b 05 fe 2d 00 00 	mov    0x2dfe(%rip),%rax        # 3fc8 <_ITM_deregisterTMCloneTable@Base>
    11ca:	48 85 c0             	test   %rax,%rax
    11cd:	74 09                	je     11d8 <_start+0x58>
    11cf:	ff e0                	jmp    *%rax
    11d1:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
    11d8:	c3                   	ret
    11d9:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
    11e0:	48 8d 3d 81 2e 00 00 	lea    0x2e81(%rip),%rdi        # 4068 <__TMC_END__>
    11e7:	48 8d 35 7a 2e 00 00 	lea    0x2e7a(%rip),%rsi        # 4068 <__TMC_END__>
    11ee:	48 29 fe             	sub    %rdi,%rsi
    11f1:	48 89 f0             	mov    %rsi,%rax
    11f4:	48 c1 ee 3f          	shr    $0x3f,%rsi
    11f8:	48 c1 f8 03          	sar    $0x3,%rax
    11fc:	48 01 c6             	add    %rax,%rsi
    11ff:	48 d1 fe             	sar    $1,%rsi
    1202:	74 14                	je     1218 <_start+0x98>
    1204:	48 8b 05 cd 2d 00 00 	mov    0x2dcd(%rip),%rax        # 3fd8 <_ITM_registerTMCloneTable@Base>
    120b:	48 85 c0             	test   %rax,%rax
    120e:	74 08                	je     1218 <_start+0x98>
    1210:	ff e0                	jmp    *%rax
    1212:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
    1218:	c3                   	ret
    1219:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
    1220:	f3 0f 1e fa          	endbr64
    1224:	80 3d 3d 2e 00 00 00 	cmpb   $0x0,0x2e3d(%rip)        # 4068 <__TMC_END__>
    122b:	75 33                	jne    1260 <_start+0xe0>
    122d:	55                   	push   %rbp
    122e:	48 83 3d aa 2d 00 00 	cmpq   $0x0,0x2daa(%rip)        # 3fe0 <__cxa_finalize@GLIBC_2.2.5>
    1235:	00 
    1236:	48 89 e5             	mov    %rsp,%rbp
    1239:	74 0d                	je     1248 <_start+0xc8>
    123b:	48 8b 3d 16 2e 00 00 	mov    0x2e16(%rip),%rdi        # 4058 <__dso_handle>
    1242:	ff 15 98 2d 00 00    	call   *0x2d98(%rip)        # 3fe0 <__cxa_finalize@GLIBC_2.2.5>
    1248:	e8 63 ff ff ff       	call   11b0 <_start+0x30>
    124d:	c6 05 14 2e 00 00 01 	movb   $0x1,0x2e14(%rip)        # 4068 <__TMC_END__>
    1254:	5d                   	pop    %rbp
    1255:	c3                   	ret
    1256:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
    125d:	00 00 00 
    1260:	c3                   	ret
    1261:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
    1268:	00 00 00 00 
    126c:	0f 1f 40 00          	nopl   0x0(%rax)
    1270:	f3 0f 1e fa          	endbr64
    1274:	e9 67 ff ff ff       	jmp    11e0 <_start+0x60>
    1279:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)

0000000000001280 <translateNumber>:
    1280:	80 3e 2d             	cmpb   $0x2d,(%rsi)
    1283:	48 c7 47 08 00 00 00 	movq   $0x0,0x8(%rdi)
    128a:	00 
    128b:	48 89 f8             	mov    %rdi,%rax
    128e:	48 c7 47 10 00 00 00 	movq   $0x0,0x10(%rdi)
    1295:	00 
    1296:	0f 94 07             	sete   (%rdi)
    1299:	c3                   	ret
    129a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)

00000000000012a0 <createTable>:
    12a0:	55                   	push   %rbp
    12a1:	53                   	push   %rbx
    12a2:	89 fb                	mov    %edi,%ebx
    12a4:	bf 10 00 00 00       	mov    $0x10,%edi
    12a9:	48 83 ec 08          	sub    $0x8,%rsp
    12ad:	e8 ee fd ff ff       	call   10a0 <malloc@plt>
    12b2:	48 63 fb             	movslq %ebx,%rdi
    12b5:	89 18                	mov    %ebx,(%rax)
    12b7:	48 c1 e7 03          	shl    $0x3,%rdi
    12bb:	48 89 c5             	mov    %rax,%rbp
    12be:	e8 dd fd ff ff       	call   10a0 <malloc@plt>
    12c3:	48 89 45 08          	mov    %rax,0x8(%rbp)
    12c7:	85 db                	test   %ebx,%ebx
    12c9:	7e 10                	jle    12db <createTable+0x3b>
    12cb:	89 da                	mov    %ebx,%edx
    12cd:	48 89 c7             	mov    %rax,%rdi
    12d0:	31 f6                	xor    %esi,%esi
    12d2:	48 c1 e2 03          	shl    $0x3,%rdx
    12d6:	e8 a5 fd ff ff       	call   1080 <memset@plt>
    12db:	48 83 c4 08          	add    $0x8,%rsp
    12df:	48 89 e8             	mov    %rbp,%rax
    12e2:	5b                   	pop    %rbx
    12e3:	5d                   	pop    %rbp
    12e4:	c3                   	ret
    12e5:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
    12ec:	00 00 00 00 

00000000000012f0 <hashCode>:
    12f0:	89 f0                	mov    %esi,%eax
    12f2:	99                   	cltd
    12f3:	f7 3f                	idivl  (%rdi)
    12f5:	89 d0                	mov    %edx,%eax
    12f7:	f7 d8                	neg    %eax
    12f9:	85 f6                	test   %esi,%esi
    12fb:	0f 49 c2             	cmovns %edx,%eax
    12fe:	c3                   	ret
    12ff:	90                   	nop

0000000000001300 <remove>:
    1300:	89 f0                	mov    %esi,%eax
    1302:	99                   	cltd
    1303:	f7 3f                	idivl  (%rdi)
    1305:	89 d0                	mov    %edx,%eax
    1307:	f7 d8                	neg    %eax
    1309:	85 f6                	test   %esi,%esi
    130b:	0f 49 c2             	cmovns %edx,%eax
    130e:	48 8b 57 08          	mov    0x8(%rdi),%rdx
    1312:	48 98                	cltq
    1314:	48 8d 0c c2          	lea    (%rdx,%rax,8),%rcx
    1318:	48 8b 39             	mov    (%rcx),%rdi
    131b:	48 85 ff             	test   %rdi,%rdi
    131e:	74 40                	je     1360 <remove+0x60>
    1320:	31 c0                	xor    %eax,%eax
    1322:	eb 0f                	jmp    1333 <remove+0x33>
    1324:	0f 1f 40 00          	nopl   0x0(%rax)
    1328:	48 89 f8             	mov    %rdi,%rax
    132b:	48 85 d2             	test   %rdx,%rdx
    132e:	74 30                	je     1360 <remove+0x60>
    1330:	48 89 d7             	mov    %rdx,%rdi
    1333:	48 8b 57 10          	mov    0x10(%rdi),%rdx
    1337:	39 37                	cmp    %esi,(%rdi)
    1339:	75 ed                	jne    1328 <remove+0x28>
    133b:	48 83 ec 08          	sub    $0x8,%rsp
    133f:	48 85 c0             	test   %rax,%rax
    1342:	74 24                	je     1368 <remove+0x68>
    1344:	48 89 50 10          	mov    %rdx,0x10(%rax)
    1348:	e8 e3 fc ff ff       	call   1030 <free@plt>
    134d:	b8 01 00 00 00       	mov    $0x1,%eax
    1352:	48 83 c4 08          	add    $0x8,%rsp
    1356:	c3                   	ret
    1357:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
    135e:	00 00 
    1360:	31 c0                	xor    %eax,%eax
    1362:	c3                   	ret
    1363:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
    1368:	48 89 11             	mov    %rdx,(%rcx)
    136b:	e8 c0 fc ff ff       	call   1030 <free@plt>
    1370:	b8 01 00 00 00       	mov    $0x1,%eax
    1375:	48 83 c4 08          	add    $0x8,%rsp
    1379:	c3                   	ret
    137a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)

0000000000001380 <insert>:
    1380:	41 55                	push   %r13
    1382:	89 f0                	mov    %esi,%eax
    1384:	41 54                	push   %r12
    1386:	55                   	push   %rbp
    1387:	48 89 d5             	mov    %rdx,%rbp
    138a:	99                   	cltd
    138b:	53                   	push   %rbx
    138c:	89 f3                	mov    %esi,%ebx
    138e:	48 83 ec 08          	sub    $0x8,%rsp
    1392:	f7 3f                	idivl  (%rdi)
    1394:	89 d0                	mov    %edx,%eax
    1396:	f7 d8                	neg    %eax
    1398:	85 f6                	test   %esi,%esi
    139a:	0f 49 c2             	cmovns %edx,%eax
    139d:	48 8b 57 08          	mov    0x8(%rdi),%rdx
    13a1:	bf 18 00 00 00       	mov    $0x18,%edi
    13a6:	48 98                	cltq
    13a8:	4c 8d 2c c2          	lea    (%rdx,%rax,8),%r13
    13ac:	4d 8b 65 00          	mov    0x0(%r13),%r12
    13b0:	e8 eb fc ff ff       	call   10a0 <malloc@plt>
    13b5:	4d 85 e4             	test   %r12,%r12
    13b8:	74 26                	je     13e0 <insert+0x60>
    13ba:	4c 89 e2             	mov    %r12,%rdx
    13bd:	eb 0a                	jmp    13c9 <insert+0x49>
    13bf:	90                   	nop
    13c0:	48 8b 52 10          	mov    0x10(%rdx),%rdx
    13c4:	48 85 d2             	test   %rdx,%rdx
    13c7:	74 17                	je     13e0 <insert+0x60>
    13c9:	39 1a                	cmp    %ebx,(%rdx)
    13cb:	75 f3                	jne    13c0 <insert+0x40>
    13cd:	48 89 6a 08          	mov    %rbp,0x8(%rdx)
    13d1:	48 83 c4 08          	add    $0x8,%rsp
    13d5:	5b                   	pop    %rbx
    13d6:	5d                   	pop    %rbp
    13d7:	41 5c                	pop    %r12
    13d9:	41 5d                	pop    %r13
    13db:	c3                   	ret
    13dc:	0f 1f 40 00          	nopl   0x0(%rax)
    13e0:	66 48 0f 6e c5       	movq   %rbp,%xmm0
    13e5:	66 49 0f 6e cc       	movq   %r12,%xmm1
    13ea:	89 18                	mov    %ebx,(%rax)
    13ec:	66 0f 6c c1          	punpcklqdq %xmm1,%xmm0
    13f0:	0f 11 40 08          	movups %xmm0,0x8(%rax)
    13f4:	49 89 45 00          	mov    %rax,0x0(%r13)
    13f8:	48 83 c4 08          	add    $0x8,%rsp
    13fc:	5b                   	pop    %rbx
    13fd:	5d                   	pop    %rbp
    13fe:	41 5c                	pop    %r12
    1400:	41 5d                	pop    %r13
    1402:	c3                   	ret
    1403:	66 66 2e 0f 1f 84 00 	data16 cs nopw 0x0(%rax,%rax,1)
    140a:	00 00 00 00 
    140e:	66 90                	xchg   %ax,%ax

0000000000001410 <lookup>:
    1410:	89 f0                	mov    %esi,%eax
    1412:	99                   	cltd
    1413:	f7 3f                	idivl  (%rdi)
    1415:	89 d0                	mov    %edx,%eax
    1417:	f7 d8                	neg    %eax
    1419:	85 f6                	test   %esi,%esi
    141b:	0f 49 c2             	cmovns %edx,%eax
    141e:	48 8b 57 08          	mov    0x8(%rdi),%rdx
    1422:	48 98                	cltq
    1424:	48 8b 04 c2          	mov    (%rdx,%rax,8),%rax
    1428:	48 85 c0             	test   %rax,%rax
    142b:	75 0c                	jne    1439 <lookup+0x29>
    142d:	c3                   	ret
    142e:	66 90                	xchg   %ax,%ax
    1430:	48 8b 40 10          	mov    0x10(%rax),%rax
    1434:	48 85 c0             	test   %rax,%rax
    1437:	74 0f                	je     1448 <lookup+0x38>
    1439:	39 30                	cmp    %esi,(%rax)
    143b:	75 f3                	jne    1430 <lookup+0x20>
    143d:	48 8b 40 08          	mov    0x8(%rax),%rax
    1441:	c3                   	ret
    1442:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
    1448:	c3                   	ret
    1449:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)

0000000000001450 <cloneString>:
    1450:	48 85 ff             	test   %rdi,%rdi
    1453:	74 43                	je     1498 <cloneString+0x48>
    1455:	55                   	push   %rbp
    1456:	53                   	push   %rbx
    1457:	48 89 fb             	mov    %rdi,%rbx
    145a:	48 83 ec 08          	sub    $0x8,%rsp
    145e:	e8 ed fb ff ff       	call   1050 <strlen@plt>
    1463:	48 8d 68 01          	lea    0x1(%rax),%rbp
    1467:	48 89 ef             	mov    %rbp,%rdi
    146a:	e8 31 fc ff ff       	call   10a0 <malloc@plt>
    146f:	48 89 c1             	mov    %rax,%rcx
    1472:	48 85 c0             	test   %rax,%rax
    1475:	74 11                	je     1488 <cloneString+0x38>
    1477:	48 89 ea             	mov    %rbp,%rdx
    147a:	48 89 de             	mov    %rbx,%rsi
    147d:	48 89 c7             	mov    %rax,%rdi
    1480:	e8 0b fc ff ff       	call   1090 <memcpy@plt>
    1485:	48 89 c1             	mov    %rax,%rcx
    1488:	48 83 c4 08          	add    $0x8,%rsp
    148c:	48 89 c8             	mov    %rcx,%rax
    148f:	5b                   	pop    %rbx
    1490:	5d                   	pop    %rbp
    1491:	c3                   	ret
    1492:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
    1498:	31 c0                	xor    %eax,%eax
    149a:	c3                   	ret
    149b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)

00000000000014a0 <stripString>:
    14a0:	48 85 ff             	test   %rdi,%rdi
    14a3:	0f 84 e7 00 00 00    	je     1590 <stripString+0xf0>
    14a9:	41 56                	push   %r14
    14ab:	41 55                	push   %r13
    14ad:	49 89 f5             	mov    %rsi,%r13
    14b0:	41 54                	push   %r12
    14b2:	55                   	push   %rbp
    14b3:	53                   	push   %rbx
    14b4:	48 85 f6             	test   %rsi,%rsi
    14b7:	0f 84 8b 00 00 00    	je     1548 <stripString+0xa8>
    14bd:	49 89 fc             	mov    %rdi,%r12
    14c0:	e8 8b fb ff ff       	call   1050 <strlen@plt>
    14c5:	4c 89 ef             	mov    %r13,%rdi
    14c8:	49 89 c6             	mov    %rax,%r14
    14cb:	e8 80 fb ff ff       	call   1050 <strlen@plt>
    14d0:	4d 85 f6             	test   %r14,%r14
    14d3:	74 73                	je     1548 <stripString+0xa8>
    14d5:	48 85 c0             	test   %rax,%rax
    14d8:	74 6e                	je     1548 <stripString+0xa8>
    14da:	31 ed                	xor    %ebp,%ebp
    14dc:	eb 0f                	jmp    14ed <stripString+0x4d>
    14de:	66 90                	xchg   %ax,%ax
    14e0:	48 83 c5 01          	add    $0x1,%rbp
    14e4:	4c 39 f5             	cmp    %r14,%rbp
    14e7:	0f 83 8b 00 00 00    	jae    1578 <stripString+0xd8>
    14ed:	41 0f be 34 2c       	movsbl (%r12,%rbp,1),%esi
    14f2:	4c 89 ef             	mov    %r13,%rdi
    14f5:	e8 66 fb ff ff       	call   1060 <strchr@plt>
    14fa:	48 85 c0             	test   %rax,%rax
    14fd:	75 e1                	jne    14e0 <stripString+0x40>
    14ff:	48 85 ed             	test   %rbp,%rbp
    1502:	75 74                	jne    1578 <stripString+0xd8>
    1504:	49 8d 5e ff          	lea    -0x1(%r14),%rbx
    1508:	48 29 eb             	sub    %rbp,%rbx
    150b:	75 09                	jne    1516 <stripString+0x76>
    150d:	eb 49                	jmp    1558 <stripString+0xb8>
    150f:	90                   	nop
    1510:	48 83 eb 01          	sub    $0x1,%rbx
    1514:	74 42                	je     1558 <stripString+0xb8>
    1516:	41 0f be 34 1c       	movsbl (%r12,%rbx,1),%esi
    151b:	4c 89 ef             	mov    %r13,%rdi
    151e:	e8 3d fb ff ff       	call   1060 <strchr@plt>
    1523:	48 85 c0             	test   %rax,%rax
    1526:	75 e8                	jne    1510 <stripString+0x70>
    1528:	48 8d 73 01          	lea    0x1(%rbx),%rsi
    152c:	4c 39 f3             	cmp    %r14,%rbx
    152f:	72 2c                	jb     155d <stripString+0xbd>
    1531:	5b                   	pop    %rbx
    1532:	4c 89 e7             	mov    %r12,%rdi
    1535:	5d                   	pop    %rbp
    1536:	41 5c                	pop    %r12
    1538:	41 5d                	pop    %r13
    153a:	41 5e                	pop    %r14
    153c:	e9 6f fb ff ff       	jmp    10b0 <realloc@plt>
    1541:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
    1548:	5b                   	pop    %rbx
    1549:	5d                   	pop    %rbp
    154a:	41 5c                	pop    %r12
    154c:	41 5d                	pop    %r13
    154e:	41 5e                	pop    %r14
    1550:	c3                   	ret
    1551:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
    1558:	be 01 00 00 00       	mov    $0x1,%esi
    155d:	41 c6 04 34 00       	movb   $0x0,(%r12,%rsi,1)
    1562:	4c 89 e7             	mov    %r12,%rdi
    1565:	5b                   	pop    %rbx
    1566:	5d                   	pop    %rbp
    1567:	41 5c                	pop    %r12
    1569:	41 5d                	pop    %r13
    156b:	41 5e                	pop    %r14
    156d:	e9 3e fb ff ff       	jmp    10b0 <realloc@plt>
    1572:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
    1578:	49 8d 56 01          	lea    0x1(%r14),%rdx
    157c:	49 8d 34 2c          	lea    (%r12,%rbp,1),%rsi
    1580:	4c 89 e7             	mov    %r12,%rdi
    1583:	48 29 ea             	sub    %rbp,%rdx
    1586:	e8 35 fb ff ff       	call   10c0 <memmove@plt>
    158b:	e9 74 ff ff ff       	jmp    1504 <stripString+0x64>
    1590:	c3                   	ret

Disassembly of section .fini:

0000000000001594 <_fini>:
    1594:	f3 0f 1e fa          	endbr64
    1598:	48 83 ec 08          	sub    $0x8,%rsp
    159c:	48 83 c4 08          	add    $0x8,%rsp
    15a0:	c3                   	ret
