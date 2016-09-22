CREATE EXTENSION shared_ispell;

-- Test ISpell dictionary with ispell affix file
CREATE TEXT SEARCH DICTIONARY shared_ispell (
                        Template=shared_ispell,
                        DictFile=ispell_sample,
                        AffFile=ispell_sample,
                        Stopwords=english
);

SELECT ts_lexize('shared_ispell', 'skies');
SELECT ts_lexize('shared_ispell', 'bookings');
SELECT ts_lexize('shared_ispell', 'booking');
SELECT ts_lexize('shared_ispell', 'foot');
SELECT ts_lexize('shared_ispell', 'foots');
SELECT ts_lexize('shared_ispell', 'rebookings');
SELECT ts_lexize('shared_ispell', 'rebooking');
SELECT ts_lexize('shared_ispell', 'unbookings');
SELECT ts_lexize('shared_ispell', 'unbooking');
SELECT ts_lexize('shared_ispell', 'unbook');

SELECT ts_lexize('shared_ispell', 'footklubber');
SELECT ts_lexize('shared_ispell', 'footballklubber');
SELECT ts_lexize('shared_ispell', 'ballyklubber');
SELECT ts_lexize('shared_ispell', 'footballyklubber');

-- Test ISpell dictionary with hunspell affix file
CREATE TEXT SEARCH DICTIONARY shared_hunspell (
                        Template=shared_ispell,
                        DictFile=ispell_sample,
                        AffFile=hunspell_sample
);

SELECT ts_lexize('shared_hunspell', 'skies');
SELECT ts_lexize('shared_hunspell', 'bookings');
SELECT ts_lexize('shared_hunspell', 'booking');
SELECT ts_lexize('shared_hunspell', 'foot');
SELECT ts_lexize('shared_hunspell', 'foots');
SELECT ts_lexize('shared_hunspell', 'rebookings');
SELECT ts_lexize('shared_hunspell', 'rebooking');
SELECT ts_lexize('shared_hunspell', 'unbookings');
SELECT ts_lexize('shared_hunspell', 'unbooking');
SELECT ts_lexize('shared_hunspell', 'unbook');

SELECT ts_lexize('shared_hunspell', 'footklubber');
SELECT ts_lexize('shared_hunspell', 'footballklubber');
SELECT ts_lexize('shared_hunspell', 'ballyklubber');
SELECT ts_lexize('shared_hunspell', 'footballyklubber');

SELECT dict_name, affix_name, words, affixes FROM shared_ispell_dicts();
SELECT stop_name, words FROM shared_ispell_stoplists();

SELECT shared_ispell_reset();

SELECT ts_lexize('shared_ispell', 'skies');
SELECT ts_lexize('shared_hunspell', 'skies');