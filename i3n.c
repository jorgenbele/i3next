/*
 * Author: Jørgen Bele Reinfjell
 * Date: xx.07.2017 [dd.mm.yyyy]
 * Modified: 26.07.2017, 12.11.2017
 * File: i3n.c
 * Description:
 *  Simple application using i3ipc-glib to interface with i3-wm
 *  and change the current workspace to another non-empty one.
 * TODO: Fix possible memory leak (as reported by valgrind).
 *
 * 26.07.2017:
 *  Refactored code.
 *
 * 12.11.2017:
 *  Added "-e" flag, which makes it select an empty workspace.
 *  Renamed next_workspace to next_workspace for readability.
 *  Added new function argument 'active' to 'next_workspace'.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <glib-object.h>
#include <i3ipc-glib/i3ipc-glib.h>

/* static bool change_to_workspace(i3ipcConnection *i3c, int num):
 *  Changes focused i3 workspace to workspace 'num'.
 * Return: true on success and false on failure. */
static bool change_to_workspace(i3ipcConnection *i3c, int num)
{
  char *cmd_str;
  size_t cmd_size;
  GSList *cmd_replies;
  GError *error;

  /* Create command string. */
  cmd_size = snprintf(NULL, 0, "workspace %i", num) + 1;
  cmd_str = malloc(cmd_size);
  if (!cmd_str) {
    perror("malloc");
    return false; /* TODO CLEANUP */
  }
  cmd_size = snprintf(cmd_str, cmd_size, "workspace %i", num);
  if (cmd_size < 0) {
    perror("snprintf");
    return false; /* TODO CLEANUP */
  }
  /* Send command to i3 asking it to change workspace. */
  error = NULL;
  cmd_replies = i3ipc_connection_command(i3c, cmd_str, &error);
  if (!cmd_replies) {
    g_fprintf(stderr, "i3ipc_con_command: %s\n", error->message);
    g_error_free(error);
    return false; /* TODO CLEANUP */
  }
  /* Cleanup. */
  free(cmd_str);
  g_slist_free_full(cmd_replies,
                    (GDestroyNotify) i3ipc_command_reply_free);
  return true;
}

/* static bool next_workspace(i3ipcConnection *i3c, bool prev,
 *                         bool force, bool active):
 *  Tries to change to the next or previous (based on bool prev) active workspace.
 *  If the bool force is set, it will change to the next workspace (based on
 *   current workspace's 'num' field.
 *  Additionally, if bool active is false it will select the
 *   first inactive workspace (direction depends on bool prev).
 * Return: true on success and false on failure. */
static bool next_workspace(i3ipcConnection *i3c, bool prev,
                           bool force, bool active)
{
  /* TODO */
  GSList *workspaces;
  GError *error;

  /* Get workspaces. */
  error = NULL;
  workspaces = i3ipc_connection_get_workspaces(i3c, &error);
  if (!workspaces) {
    g_fprintf(stderr, "i3ipc_connection_get_workspaces: %s\n",
              error->message);
    g_error_free(error);
    return false;
  }
  /* Reverse list of workspaces. */
  workspaces = g_slist_reverse(workspaces);

  if (!active) {
    GSList *iter, *niter;
    /* If active is false, find the first inactive workspace by number. */
    for (iter = workspaces; iter != NULL && iter->data;
         iter = niter) {
      niter = g_slist_next(iter);
      if (niter && niter->data) {
#define WNUM(iter) (((i3ipcWorkspaceReply*)((iter)->data))->num)
        int ndiff = WNUM(niter) - WNUM(iter);
        if (ndiff > 1) {
          if (prev) {
            return change_to_workspace(i3c, WNUM(niter)-1);
          }
          return change_to_workspace(i3c, WNUM(iter)+1);
        }
#undef WNUM
      }
    }
    /* Fall-through!. */
  }

  /* Traverse list, find current workspace. */
  GSList *iterator;
  i3ipcWorkspaceReply *workspace = NULL;
  for (iterator = workspaces; iterator != NULL && iterator->data;
       iterator = g_slist_next(iterator)) {
    workspace = iterator->data;
    if (workspace->focused) {
      break;
    }
  }

  bool ret = false;
  if (force) {
    /* If force is true, we just need to change
     * to the prev/next workspace. */
    if (workspace) {
      ret = change_to_workspace(i3c, workspace->num + (prev? -1 : 1));
    }
  } else {
    /* Select workspace element to change to in list. */
    if (prev) {
      iterator = g_slist_nth(workspaces,
                             g_slist_position(workspaces, iterator) - 1);
    } else {
      iterator = g_slist_next(iterator);
    }

    if (!iterator) {
      ret = false;
      fprintf(stderr, "Unable to change to %s active workspace\n",
              prev ? "previous" : "next");
    } else {
      workspace = iterator->data;
      ret = change_to_workspace(i3c, workspace->num);
    }
  }
  /* Cleanup. */
  g_slist_free_full(workspaces, (GDestroyNotify) i3ipc_workspace_reply_free);
  return ret;
}

static void usage(const char *prog)
{
  printf("Usage: %s [-efhnp] \n", prog);
  printf("  -e\tFind the nearest empty workspace and select it\n");
  printf("  -f\tForce change to next workspace"
         " (even if none is active)\n");
  printf("  -h\tShows this message and quits\n");
  printf("  -n\tTry to change to the next workspace\n");
  printf("  -p\tTry to change to the previous workspace\n");
}

int main(int argc, char *argv[])
{
  /* Flags. */
  bool prev   = false;
  bool force  = false;
  bool active = true;

  int opt;
  /* Parse opts using getopt. */
  while ((opt = getopt(argc, argv, "efhnp")) != -1) {
    switch (opt) {
    case 'f':
      force = true;
      break;
    case 'h':
      usage(argv[0]);
      exit(EXIT_SUCCESS);
      break;
    case 'p':
      prev = true;
      break;
    case 'n':
      prev = false;
      break;
    case 'e':
      active = false;
      force = true;
      break;
    default:
      fprintf(stderr, "Unknown option: %c\n", opt);
      exit(EXIT_FAILURE);
      break;
    }
  }
  /* Initialize i3. */
  i3ipcConnection *i3c;
  GError *error = NULL;
  i3c = i3ipc_connection_new(NULL, &error);
  if (!i3c) {
    g_fprintf(stderr, "%ul i3ipc_connection_new failed: %s\n",
              __LINE__ - 1, error->message);
    g_error_free(error);
    exit(EXIT_FAILURE);
  }
  /* Goto next workspace. */
  bool ret = next_workspace(i3c, prev, force, active);
  if (!ret) {
    fprintf(stderr, "Failed to change workspace!\n");
  }

  /* Cleanup i3. */
  g_object_unref(i3c);

  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
