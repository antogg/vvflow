VVFlow. Инструкция пользователя
=======

Установка
---------

На данный момент единственным вариантом установки комплекса является сборка из исходников.
Для этого используйте команду

    $ make all

По умолчанию установка производится в домашней папке.

    $ make install

После этих манипуляций комплекс расползется по директориям

 - `~/.local/bin` - бинарники и скрипты
 - `~/.local/lib` - библиотеки
 - `~/.local/share/vvhd` - автодополнение bash

Так как комплекс устанавливается в непривычные для системы места необходимо подправить конфиги. Добавьте следующие строчки в конец файла `~/.bashrc`

    export PATH=$HOME/.local/bin:$PATH
    export LD_LIBRARY_PATH=$HOME/.local/lib/:$LD_LIBRARY_PATH
    [[ -f $HOME/.local/share/vvhd/bash_completion ]] && source $HOME/.local/share/vvhd/bash_completion

И перезагрузите его командой

    $ source ~/.bashrc

Использование
-------------

*vvcompose*
-----------

Для того, что бы что-то посчитать, первым дело нужно создать файл расчета. Этим занимается программа `vvcompose`

    usage: vvcompose COMMAND ARGS [...]

    COMMAND:
        load WHAT FILENAME
            импортирует данные из файла
            возможные варианты: hdf, body, vort, ink, ink_source
        save FILENAME
            сохраняет подготовленный файл расчета
        del  WHAT
            удаляет указанные сущности из файла
            возможные варианты: vort, ink, ink_source
            также можно удалить тело с нужным номером, например: body00
        set  VARIABLE VALUE
            устанавливает значение заданного параметра

Для команды `load` понадобится подготовленный файл с данными. Формат файла зависит от того, что мы собираемся загружать.

 - `hdf`: тот самый файл расчета, предварительно созданный при помощи `vvcompose` или сохраненный при расчете программой `vvflow`
 - `body`: простой текстовый файл, в две колонки `x y`, координаты вершин тела
 - `vort`: простой текстовый файл, в три колонки `x y g`, третья колонка соответствует циркуляции вихрей
 - `ink`, `ink_source`: простой текстовый файл, в три колонки `x y id`, третья колонка на результатах расчета не сказывается, но может пригодиться при обработке результатов

Вот так выглядит расчет цилиндра

    $ vvcompose \
        set caption re600_n350 \
        load body cyl350.txt \
        set inf_speed.x '1' \
        set dt 0.005 \
        set dt_save 0.1 \
        set re 600 \
        set time 0 \
        set time_to_finish 500 \
        save re600_n350.h5

Вместо того, что бы загружать тело из файла, можно соорудить удобный костылик на баше

    function gen_cylinder() {
    awk -v N=$1 -f - /dev/null <<AWK
        BEGIN{pi=atan2(0, -1)}
        END{
            for(i=0;i<N;i++) {
                x =  0.5*cos(2*pi*i/N)
                y = -0.5*sin(2*pi*i/N)
                printf("%+0.5f %+0.5f\n", x, y)
            }
        }
    AWK
    }

    vvcompose load body <(gen_cylinder 350)

Аналогичным способом можно генерировать и другие данные. За примерами стоит заглянуть в папку с тестами. Там можно найти много интересного.

*vvflow*
--------

Следующим этапом является непосредственно запуск расчета. Расчетом занимается программа `vvflow`. Если не углубляться в подробности, то все выглядит просто:

    $ vvflow re600_n350.h5

Кулеры зажужжат, в текущей директории начнут появляться результаты, и через денёк другой можно будет заняться обработкой. Если перспектива ждать не радует, а под рукой есть кластер c [PBS](https://en.wikipedia.org/wiki/Portable_Batch_System), то запуск будет немного сложнее.
    
    $ echo 'cd "${PBS_O_WORKDIR}"; export PATH="${PBS_O_PATH}"; vvflow re600_n350.h5;' | qsub -l nodes=1:ppn=12 -N testrun
