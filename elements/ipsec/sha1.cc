/*
 * sha1.{cc,hh} -- element implements IPsec SHA1 authentication (RFC 2404)
 * Benjie Chen
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Further elaboration of this license, including a DISCLAIMER OF ANY
 * WARRANTY, EXPRESS OR IMPLIED, is provided in the LICENSE file, which is
 * also accessible at http://www.pdos.lcs.mit.edu/click/license.html
 */

#include <click/config.h>
#ifndef HAVE_IPSEC
# error "Must #define HAVE_IPSEC in config.h"
#endif
#include "sha1.hh"
#include "esp.hh"
#include <click/ipaddress.hh>
#include <click/confparse.hh>
#include <click/click_ip.h>
#include <click/error.hh>
#include <click/glue.hh>
#include "elements/ipsec/sha1_impl.hh"

#define SHA_DIGEST_LEN 20

IPsecAuthSHA1::IPsecAuthSHA1()
{
  add_input();
  add_output();
}

IPsecAuthSHA1::~IPsecAuthSHA1()
{
}

void
IPsecAuthSHA1::notify_noutputs(int n)
{ 
  set_noutputs(n);
}

IPsecAuthSHA1 *
IPsecAuthSHA1::clone() const
{
  return new IPsecAuthSHA1();
}

int
IPsecAuthSHA1::configure(const Vector<String> &conf, ErrorHandler *errh)
{
  if (cp_va_parse(conf, this, errh,
		  cpInteger, "Compute/Verify (0/1)", &_op, 0) < 0)
    return -1;
  return 0;
}

int
IPsecAuthSHA1::initialize(ErrorHandler *)
{
  _drops = 0;
  return 0;
}


Packet *
IPsecAuthSHA1::simple_action(Packet *p)
{
  // compute sha1
  if (_op == COMPUTE_AUTH) {
    unsigned char digest [SHA_DIGEST_LEN];
    SHA1_ctx ctx;
    SHA1_init (&ctx);
    SHA1_update (&ctx, (u_char*) p->data(), p->length());
    SHA1_final (digest, &ctx);
    WritablePacket *q = p->put(12);
    u_char *ah = ((u_char*)q->data())+q->length()-12;
    memmove(ah, digest, 12);
    return q;
  } 
  
  else {
    const u_char *ah = p->data()+p->length()-12;
    unsigned char digest [SHA_DIGEST_LEN];
    SHA1_ctx ctx;
    SHA1_init (&ctx);
    SHA1_update (&ctx, (u_char*) p->data(), p->length()-12);
    SHA1_final (digest, &ctx);
    if (memcmp(ah, digest, 12)) {
      if (_drops == 0) 
	click_chatter("Invalid SHA1 authentication digest");
      _drops++;
      if (noutputs() > 1) 
	output(1).push(p);
      else 
	p->kill(); 
      return 0;
    }
    p->take(12);
    return p;
  }
}

String
IPsecAuthSHA1::drop_handler(Element *e, void *)
{
  IPsecAuthSHA1 *a = (IPsecAuthSHA1 *)e;
  return String(a->_drops) + "\n";
}

void
IPsecAuthSHA1::add_handlers()
{
  add_read_handler("drops", drop_handler, 0);
}

EXPORT_ELEMENT(IPsecAuthSHA1)
ELEMENT_MT_SAFE(IPsecAuthSHA1)

