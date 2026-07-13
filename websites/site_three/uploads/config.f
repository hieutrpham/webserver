if status is-interactive
    # Commands to run in interactive sessions can go here
    set -U fish_autosuggestion_enabled 1
    set -U fish_term24bit 1
end

fish_config prompt choose arrow
fish_add_path ~/opt/go/bin

function fcd
    set dir (find . -type d | fzf)
    if test -n "$dir"
        cd "$dir"
    end
end

function y
	set tmp (mktemp -t "yazi-cwd.XXXXXX")
	yazi $argv --cwd-file="$tmp"
	if set cwd (command cat -- "$tmp"); and [ -n "$cwd" ]; and [ "$cwd" != "$PWD" ]
		builtin cd -- "$cwd"
	end
	rm -f -- "$tmp"
end

#calling tmux
function t
  if tmux list-sessions &>/dev/null
    tmux attach
  else
    tmux
  end
end


alias ls='ls --color=auto'
alias grep='grep --color=auto'
alias v='nvim'
alias gs="git status"
alias ga="git add ."
alias gc="git commit -m"
alias gp="git push"
alias gl="git pull --rebase"
alias nf='fzf -m --preview="bat {}" --bind "enter:become(nvim {+})"'
alias f='ft_lock'
alias cat='bat'
abbr --add val valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all --trace-children=yes --track-fds=yes
abbr --add make compiledb make
abbr --add lls ls

setxkbmap -option caps:none

export MANPAGER="sh -c 'sed -u -e \"s/\\x1B\[[0-9;]*m//g; s/.\\x08//g\" | bat -p -lman'"
set -gx EDITOR nvim
zoxide init --cmd cd fish | source
