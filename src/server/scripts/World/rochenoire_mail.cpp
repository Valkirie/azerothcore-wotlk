/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "DatabaseEnv.h"
#include "Mail.h"
#include "Player.h"
#include "PlayerScript.h"
#include "StringFormat.h"
#include "WorldSession.h"

enum RochenoireMail
{
    ROCHENOIRE_MAIL_LEVEL_10              = 10,
    ROCHENOIRE_MAIL_BODY_HORDE_LEVEL_10   = 21019,
    ROCHENOIRE_MAIL_BODY_ALLIANCE_LEVEL_10 = 21020,
    ROCHENOIRE_MAIL_SUBJECT_HORDE_LEVEL_10 = 21021,
    ROCHENOIRE_MAIL_SUBJECT_ALLIANCE_LEVEL_10 = 21022,
    ROCHENOIRE_MAIL_SUBJECT_WELCOME       = 21023,
    ROCHENOIRE_MAIL_BODY_WELCOME          = 21024,
    NPC_THRALL                            = 4949,
    NPC_KING_VARIAN_WRYNN                 = 29611,
};

class RochenoireMailPlayerScript : public PlayerScript
{
public:
    RochenoireMailPlayerScript() : PlayerScript("RochenoireMailPlayerScript", { PLAYERHOOK_ON_LEVEL_CHANGED, PLAYERHOOK_ON_FIRST_LOGIN }) { }

    void OnPlayerLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (oldLevel < ROCHENOIRE_MAIL_LEVEL_10 && player->GetLevel() >= ROCHENOIRE_MAIL_LEVEL_10)
            SendLevel10Mail(player);
    }

    void OnPlayerFirstLogin(Player* player) override
    {
        SendMail(player, ROCHENOIRE_MAIL_SUBJECT_WELCOME, ROCHENOIRE_MAIL_BODY_WELCOME);

        if (player->GetLevel() >= ROCHENOIRE_MAIL_LEVEL_10)
            SendLevel10Mail(player);
    }

private:
    static void SendLevel10Mail(Player* player)
    {
        if (player->GetTeamId() == TEAM_ALLIANCE)
            SendMail(player, ROCHENOIRE_MAIL_SUBJECT_ALLIANCE_LEVEL_10, ROCHENOIRE_MAIL_BODY_ALLIANCE_LEVEL_10);
        else
            SendMail(player, ROCHENOIRE_MAIL_SUBJECT_HORDE_LEVEL_10, ROCHENOIRE_MAIL_BODY_HORDE_LEVEL_10);
    }

    static void SendMail(Player* player, uint32 subjectEntry, uint32 bodyEntry)
    {
        WorldSession* session = player->GetSession();
        if (!session)
            return;

        std::string subject = session->GetAcoreString(subjectEntry);
        std::string body = Acore::StringFormat(session->GetAcoreString(bodyEntry), player->GetName());
        uint32 senderEntry = player->GetTeamId() == TEAM_ALLIANCE ? NPC_KING_VARIAN_WRYNN : NPC_THRALL;

        CharacterDatabaseTransaction transaction = CharacterDatabase.BeginTransaction();
        MailDraft(subject, body).SendMailTo(transaction, MailReceiver(player), MailSender(MAIL_CREATURE, senderEntry), MAIL_CHECK_MASK_HAS_BODY);
        CharacterDatabase.CommitTransaction(transaction);
    }
};

void AddSC_rochenoire_mail()
{
    new RochenoireMailPlayerScript();
}
