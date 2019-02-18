//
//  main.c
//  eestsetset
//
//  Created by Rory B. Bellows on 23/10/2017.
//  Copyright © 2017 Rory B. Bellows. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <bsm/libbsm.h>
#include <sys/ioctl.h>
#include <security/audit/audit_ioctl.h>
#include <libproc.h>
#include <unistd.h>

#define AUT_FLAG_NO  0x00000000 // Invalid Class (no)
#define AUT_FLAG_FR  0x00000001 // File read (fr)
#define AUT_FLAG_FW  0x00000002 // File write (fw)
#define AUT_FLAG_FA  0x00000004 // File attribute access (fa)
#define AUT_FLAG_FM  0x00000008 // File attribute modify (fm)
#define AUT_FLAG_FC  0x00000010 // File create (fc)
#define AUT_FLAG_FD  0x00000020 // File delete (fd)
#define AUT_FLAG_CL  0x00000040 // File close (cl)
#define AUT_FLAG_PC  0x00000080 // Process (pc)
#define AUT_FLAG_NT  0x00000100 // Network (nt)
#define AUT_FLAG_IP  0x00000200 // IPC (ip)
#define AUT_FLAG_NA  0x00000400 // Non attributable (na)
#define AUT_FLAG_AD  0x00000800 // Administrative (ad)
#define AUT_FLAG_LO  0x00001000 // Login/Logout (lo)
#define AUT_FLAG_AA  0x00002000 // Authentication and authorization (aa)
#define AUT_FLAG_AP  0x00004000 // Application (ap)
#define AUT_FLAG_IO  0x20000000 // ioctl (io)
#define AUT_FLAG_EX  0x40000000 // exec (ex)
#define AUT_FLAG_OT  0x80000000 // Miscellaneous (ot)
#define AUT_FLAG_ALL 0xFFFFFFFF // All flags set (all)

typedef struct {
  char* f_path;
  char* proc_path;
  pid_t proc_pid;
  char* username;
  int e_type;
} audit_event_t;

int main(int argc, const char * argv[]) {
  if (geteuid() != 0)
    return 1; // Required root
  
  FILE* audit_fh = fopen("/dev/auditpipe", "r");
  if (!audit_fh)
    return 1;
  int audit_fh_no = fileno(audit_fh);
  
  int mode = AUDITPIPE_PRESELECT_MODE_LOCAL;
  int ioctl_r = ioctl(audit_fh_no, AUDITPIPE_SET_PRESELECT_MODE, &mode);
  if (ioctl_r == -1)
    return 1;
  
  int q_len;
  ioctl_r = ioctl(audit_fh_no, AUDITPIPE_GET_QLIMIT_MAX, &q_len);
  if (ioctl_r == -1)
    return 1;
  ioctl_r = ioctl(audit_fh_no, AUDITPIPE_SET_QLIMIT, &q_len);
  if (ioctl_r == -1)
    return 1;
  
  u_int e_mask = AUT_FLAG_ALL;
  
  u_int attrib_e_mask = e_mask;
  ioctl_r = ioctl(audit_fh_no, AUDITPIPE_SET_PRESELECT_FLAGS, &attrib_e_mask);
  if (ioctl_r == -1)
    return 1;
  
  u_int non_attrib_e_mask = e_mask;
  ioctl_r = ioctl(audit_fh_no, AUDITPIPE_SET_PRESELECT_NAFLAGS, &non_attrib_e_mask);
  if (ioctl_r == -1)
    return 1;
  
  u_char* buffer;
  audit_event_t event;
  tokenstr_t token;
  while (true) {
    memset(&event, 0, sizeof(event));
    int len = au_read_rec(audit_fh, &buffer);
    if (len == -1)
      return 1;
    
    int p_len = 0;
    while (len) {
      if (au_fetch_tok(&token, buffer + p_len, len) == -1)
        return 1;
      
      au_print_tok(stdout, &token, "\n", 0, 0);
      
      switch (token.id) {
        case AUT_HEADER32:
          break;
        case AUT_HEADER32_EX:
          break;
        case AUT_HEADER64:
          break;
        case AUT_HEADER64_EX:
          break;
        case AUT_TRAILER:
          break;
        case AUT_ARG32:
          break;
        case AUT_ARG64:
          break;
        case AUT_DATA:
          break;
        case AUT_ATTR32:
          break;
        case AUT_ATTR64:
          break;
        case AUT_EXIT:
          break;
        case AUT_EXEC_ARGS:
          break;
        case AUT_EXEC_ENV:
          break;
        case AUT_OTHER_FILE32:
          break;
        case AUT_NEWGROUPS:
          break;
        case AUT_IN_ADDR:
          break;
        case AUT_IN_ADDR_EX:
          break;
        case AUT_IP:
          break;
        case AUT_IPC:
          break;
        case AUT_IPC_PERM:
          break;
        case AUT_IPORT:
          break;
        case AUT_OPAQUE:
          break;
        case AUT_PATH:
          break;
        case AUT_PROCESS32:
          break;
        case AUT_PROCESS32_EX:
          break;
        case AUT_PROCESS64:
          break;
        case AUT_PROCESS64_EX:
          break;
        case AUT_RETURN32:
          break;
        case AUT_RETURN64:
          break;
        case AUT_SEQ:
          break;
        case AUT_SOCKET:
          break;
        case AUT_SOCKINET32:
          break;
        case AUT_SOCKUNIX:
          break;
        case AUT_SUBJECT32:
          break;
        case AUT_SUBJECT64:
          break;
        case AUT_SUBJECT32_EX:
          break;
        case AUT_SUBJECT64_EX:
          break;
        case AUT_TEXT:
          break;
        case AUT_SOCKET_EX:
          break;
        case AUT_ZONENAME:
          break;
        default:
          break;
      }
      
      p_len += token.len;
      len   -= token.len;
    }
    free(buffer);
    return 1;
  }
  return 0;
}
