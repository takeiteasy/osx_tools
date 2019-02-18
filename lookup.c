//
//  main.c
//  testeststeset
//
//  Created by Rory B. Bellows on 22/10/2017.
//  Copyright © 2017 Rory B. Bellows. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <libproc.h>
#include <sys/proc_info.h>

void pid_info(int pid) {
  struct proc_bsdinfo proc;
  proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &proc, PROC_PIDTBSDINFO_SIZE);
  printf("%s: %d\n", proc.pbi_name, pid);
  
  int pid_size = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, 0, 0);
  if (pid_size < 0) {
    puts("FATAL: proc_pidinfo(PROC_PIDLISTFDS) failed.");
    exit(EXIT_FAILURE);
  }
  
  struct proc_fdinfo* fd_info = (struct proc_fdinfo*)malloc(pid_size);
  if (!fd_info) {
    puts("FATAL: malloc() failed.");
    exit(EXIT_FAILURE);
  }
  
  proc_pidinfo(pid, PROC_PIDLISTFDS, 0, fd_info, pid_size);
  struct vnode_fdinfowithpath node_info;
  struct socket_fdinfo sock_info;
  struct pipe_fdinfo pipe_info;
  struct appletalk_info atalk_info;
  for (int i = 0; i < (pid_size / PROC_PIDLISTFD_SIZE); ++i) {
    if (fd_info[i].proc_fdtype == PROX_FDTYPE_VNODE) {
      if (proc_pidfdinfo(pid, fd_info[i].proc_fd, PROC_PIDFDVNODEPATHINFO, &node_info, PROC_PIDFDVNODEPATHINFO_SIZE) == PROC_PIDFDVNODEPATHINFO_SIZE)
        printf("\t%d FILE: %s\n", fd_info[i].proc_fd, node_info.pvip.vip_path);
    } else if (fd_info[i].proc_fdtype == PROX_FDTYPE_SOCKET) {
      if (proc_pidfdinfo(pid, fd_info[i].proc_fd, PROC_PIDFDSOCKETINFO, &sock_info, PROC_PIDFDSOCKETINFO_SIZE) == PROC_PIDFDSOCKETINFO_SIZE) {
        if (sock_info.psi.soi_family == AF_INET) {
          int local, remote, tcp = 1;
          if (sock_info.psi.soi_kind == SOCKINFO_TCP) {
            local  = (int)ntohs(sock_info.psi.soi_proto.pri_tcp.tcpsi_ini.insi_lport);
            remote = (int)ntohs(sock_info.psi.soi_proto.pri_tcp.tcpsi_ini.insi_fport);
          } else {
            local  = (int)ntohs(sock_info.psi.soi_proto.pri_in.insi_lport);
            remote = (int)ntohs(sock_info.psi.soi_proto.pri_in.insi_fport);
            tcp = 0;
          }
          if (remote == 0)
            printf("\t%d LISTEN%s: %d\n", fd_info[i].proc_fd, (tcp ? " TCP" : ""), local);
          else
            printf("\t%d OPEN%s: %d -> %d", fd_info[i].proc_fd, (tcp ? " TCP" : ""), local, remote);
        }
      }
    } else if (fd_info[i].proc_fdtype == PROX_FDTYPE_PIPE) {
      if (proc_pidinfo(pid, fd_info[i].proc_fd, PROC_PIDFDPIPEINFO, &pipe_info, PROC_PIDFDPIPEINFO_SIZE) == PROC_PIDFDPIPEINFO_SIZE)
        printf("\tPIPE: %llu\n", pipe_info.pipeinfo.pipe_peerhandle);
    } else if (fd_info[i].proc_fd == PROX_FDTYPE_ATALK) {
      if (proc_pidinfo(pid, fd_info[i].proc_fd, PROC_PIDFDATALKINFO, &atalk_info, PROC_PIDFDATALKINFO_SIZE) == PROC_PIDFDATALKINFO_SIZE)
        printf("\t%d ATALK\n", fd_info[i].proc_fd);
    }
  }
  free(fd_info);
}

int main(int argc, const char* argv[]) {
  size_t size_pids = proc_listpids(PROC_ALL_PIDS, 0, NULL, 0);
  pid_t pids[2 * size_pids / sizeof(pid_t)];
  size_pids = proc_listpids(PROC_ALL_PIDS, 0, pids, (int)sizeof(pids));
  size_t len_pids = size_pids / sizeof(pid_t);
  
  for (int i = 0; i < len_pids; ++i)
    pid_info(pids[i]);
  
  return 0;
}
