CREATE EXTENSION referee;

SELECT * FROM referee.decision;

SELECT referee.get_winner(1);
SELECT referee.get_winner(2);
SELECT referee.get_winner(4);
SELECT referee.get_winner(1);
SELECT * FROM referee.decision;

SELECT referee.clean();
SELECT referee.get_winner(4);
SELECT referee.get_winner(2);
SELECT referee.get_winner(1);
SELECT referee.get_winner(4);
SELECT * FROM referee.decision;

SELECT referee.clean();
SELECT referee.clean();
SELECT * FROM referee.decision;
