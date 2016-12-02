function turn_indicator_on(id)
{
	$('#'+id).css({ display: "block" });
}

function turn_indicator_off(id)
{
	$('#'+id).css({ display: "none" });
}

function draw_active_jobs()
{
	turn_indicator_on('aj-ind');
	$.ajax('/json/active_jobs.json', {
		success: function (data) {
			var T = $('#aj').find('tbody');
			T.empty();
			var table = [];

			for(i=0; i < data.length; i++)
			{
				var row = [];

				make_td(row, data[i].cron, "td-dig");
				make_td(row, strip_date(data[i].scheduled_at, 'min'), "time");
				make_td(row, strip_date(data[i].started, 'sec'), "time");
				make_td(row, data[i].elapsed);
				make_td(row, data[i].name);
				make_td(row, data[i].run_as);
				make_td(row, data[i].owner);

				table[table.length] = row;
			}
			make_table_body(T, table);
			turn_indicator_off('aj-ind');
			setTimeout(function () { draw_active_jobs() }, 1000);
		},
		error: function (xhr, status, text) {
			alert(status);
		}
	});
}

function draw_processed_jobs()
{
	turn_indicator_on('aj-ind');
	$.ajax('/json/log.json', {
		success: function (data) {
			var T = $('#pj').find('tbody');
			T.empty();
			var table = [];

			for(i=0; i < data.length; i++)
			{
				var row = [];

				make_td(row, data[i].cron, "td-dig");
				make_td(row, strip_date(data[i].scheduled_at, 'min'), "time");
				make_td(row, strip_date(data[i].started, 'sec'), "time");
				make_td(row, strip_date(data[i].finished, 'sec'), "time");
				make_td(row, data[i].name);
				make_td(row, data[i].run_as);
				make_td(row, data[i].owner);
				make_td(row, data[i].status, data[i].status);
				make_td(row, clean_text(data[i].message));

				table[table.length] = row;
			}
			make_table_body(T, table);
			turn_indicator_off('aj-ind');
			setTimeout(function () { draw_processed_jobs() }, 4000);
		},
		error: function (xhr, status, text) {
			alert(status);
		}
	});
}

function strip_date(text, base)
{
	if(!text) return '';
	if(text == 'null') return '';
	if(base == 'min') return text.substr(0,16);
	if(base == 'sec') return text.substr(0,19);

	return text;
}

function clean_text(text)
{
	if(!text) return '';
	if(text == 'null') return '';
	return text;
}

function make_table_body(table, data)
{
	for(var i=0; i < data.length; i++)
	{
		var tr = $('<tr/>');
		for(var j=0; j < data[i].length; j++)
		{
			tr.append(data[i][j]);
		}
		table.append(tr);
	}
}

function make_td(rows, text, className)
{
	var td = '<td';
	if(className) td += ' class="'+className+'"';
	td += '>'+text+'</td>';

	rows[rows.length] = $(td);
}


$(function() {
	draw_active_jobs();
	draw_processed_jobs();
});
