<?php
/**
 * Plugin Name:       Keller54 Update-Newsletter
 * Description:       Schickt angemeldeten Mitgliedern eine E-Mail, sobald es eine neue Version eines Keller54-Plugins gibt – nur wer es ausdrücklich möchte (Double-Opt-in), mit Abmelde-Link in jeder Mail.
 * Version:           1.0.0
 * Requires at least: 6.0
 * Requires PHP:      7.4
 * Author:            Keller54
 * License:           GPL-2.0-or-later
 */

if ( ! defined( 'ABSPATH' ) ) {
	exit;
}

class K54NL {

	const VERSION   = '1.0.0';
	const OPT       = 'k54nl_settings';
	const OPT_SENT  = 'k54nl_sent';      // slug => zuletzt angekündigte Version
	const OPT_LOG   = 'k54nl_log';
	const BATCH     = 25;                // Mails pro Durchlauf (Hoster-freundlich)

	// Nutzer-Metadaten
	const M_STATUS  = 'k54nl_status';    // '' | pending | active
	const M_TOKEN   = 'k54nl_token';
	const M_CONSENT = 'k54nl_consent';   // Zeitpunkt der Bestätigung (Nachweis)
	const M_ASKED   = 'k54nl_asked';     // Zeitpunkt der Anfrage

	public static function init() {
		add_shortcode( 'keller54_newsletter', array( __CLASS__, 'shortcode' ) );
		add_filter( 'the_content', array( __CLASS__, 'append_to_member_page' ), 20 );
		add_action( 'init', array( __CLASS__, 'handle_links' ) );
		add_action( 'admin_post_k54nl_toggle', array( __CLASS__, 'handle_toggle' ) );
		add_action( 'update_option_k54u_products', array( __CLASS__, 'products_changed' ), 10, 2 );
		add_action( 'add_option_k54u_products', array( __CLASS__, 'products_added' ), 10, 2 );
		add_action( 'k54nl_send', array( __CLASS__, 'send_batch' ), 10, 4 );
		add_action( 'admin_menu', array( __CLASS__, 'menu' ) );
		add_action( 'admin_post_k54nl_save', array( __CLASS__, 'save_settings' ) );
		add_action( 'admin_post_k54nl_manual', array( __CLASS__, 'manual_send' ) );
		add_action( 'rest_api_init', array( __CLASS__, 'routes' ) );
		add_action( 'delete_user', array( __CLASS__, 'forget_user' ) );
	}

	// ------------------------------------------------------------ Einstellungen

	public static function settings() {
		$d = array(
			'auto'     => 1,                 // bei neuer Version automatisch senden
			'products' => array(),           // leer = alle Produkte
		);
		$s = get_option( self::OPT, array() );
		return wp_parse_args( is_array( $s ) ? $s : array(), $d );
	}

	private static function log( $text ) {
		$log = get_option( self::OPT_LOG, array() );
		$log = is_array( $log ) ? $log : array();
		array_unshift( $log, current_time( 'mysql' ) . ' · ' . $text );
		update_option( self::OPT_LOG, array_slice( $log, 0, 30 ), false );
	}

	private static function member_url() {
		if ( function_exists( 'k54m_area_url' ) ) {
			return k54m_area_url();
		}
		return home_url( '/mein-bereich/' );
	}

	private static function status( $user_id ) {
		return (string) get_user_meta( $user_id, self::M_STATUS, true );
	}

	private static function token( $user_id ) {
		$t = (string) get_user_meta( $user_id, self::M_TOKEN, true );
		if ( strlen( $t ) < 20 ) {
			$t = wp_generate_password( 32, false );
			update_user_meta( $user_id, self::M_TOKEN, $t );
		}
		return $t;
	}

	private static function link( $action, $user_id ) {
		return add_query_arg(
			array(
				$action => self::token( $user_id ),
				'u'     => (int) $user_id,
			),
			home_url( '/' )
		);
	}

	// ------------------------------------------------------------ Frontend-Kasten

	public static function append_to_member_page( $content ) {
		if ( ! is_singular() || ! in_the_loop() || ! is_main_query() ) {
			return $content;
		}
		$pid = function_exists( 'k54m_setting' ) ? (int) k54m_setting( 'page_id' ) : 0;
		if ( ( $pid && is_page( $pid ) ) || is_page( 'mein-bereich' ) ) {
			if ( false === strpos( $content, 'k54nl-box' ) ) {
				$content .= self::shortcode();
			}
		}
		return $content;
	}

	public static function shortcode() {
		$msg = isset( $_GET['k54nl'] ) ? sanitize_key( wp_unslash( $_GET['k54nl'] ) ) : ''; // phpcs:ignore WordPress.Security.NonceVerification
		$texts = array(
			'sent'    => 'Fast geschafft: Wir haben dir eine E-Mail geschickt. Bitte klicke dort auf den Bestätigungs-Link.',
			'ok'      => 'Danke! Du bekommst ab jetzt eine E-Mail, sobald es ein Update gibt.',
			'off'     => 'Du bist abgemeldet und bekommst keine Update-Mails mehr.',
			'invalid' => 'Der Link ist nicht (mehr) gültig. Bitte melde dich im Mitgliederbereich neu an.',
		);
		ob_start();
		?>
		<style>
		.k54nl-box{max-width:760px;margin:28px auto;border:1px solid rgba(212,175,106,.35);border-radius:14px;padding:20px 24px;background:rgba(212,175,106,.06);color:#f3eee4}
		.k54nl-box h3{margin:0 0 6px;color:#d4af6a;font-size:15px;letter-spacing:.18em;text-transform:uppercase}
		.k54nl-box p{margin:6px 0;opacity:.9;font-size:15px;line-height:1.5}
		.k54nl-box .k54nl-small{font-size:12.5px;opacity:.65}
		.k54nl-box .k54nl-msg{border-left:3px solid #d4af6a;padding-left:10px;margin:10px 0}
		.k54nl-box button{background:#d4af6a;color:#0b0b0b;border:0;border-radius:8px;padding:9px 18px;font-weight:700;letter-spacing:.06em;cursor:pointer}
		.k54nl-box button.k54nl-off{background:transparent;color:#d4af6a;border:1px solid #d4af6a}
		</style>
		<div class="k54nl-box" id="update-newsletter">
			<h3>Update-Newsletter</h3>
			<?php if ( $msg && isset( $texts[ $msg ] ) ) : ?>
				<p class="k54nl-msg"><?php echo esc_html( $texts[ $msg ] ); ?></p>
			<?php endif; ?>
			<?php if ( ! is_user_logged_in() ) : ?>
				<p>Melde dich an, dann kannst du dich hier für eine E-Mail bei neuen Plugin-Versionen eintragen.</p>
			<?php else :
				$uid = get_current_user_id();
				$st  = self::status( $uid );
				?>
				<?php if ( 'active' === $st ) : ?>
					<p><strong>Eingeschaltet.</strong> Du bekommst eine E-Mail, sobald es eine neue Version des K-54, K-54 S oder eines anderen Keller54-Plugins gibt.</p>
				<?php elseif ( 'pending' === $st ) : ?>
					<p><strong>Noch nicht bestätigt.</strong> Bitte klicke auf den Link in der E-Mail an <?php echo esc_html( wp_get_current_user()->user_email ); ?>.</p>
				<?php else : ?>
					<p>Möchtest du per E-Mail erfahren, wenn es ein Update für meine Plugins gibt? Höchstens eine Mail pro neuer Version, keine Werbung.</p>
				<?php endif; ?>
				<form method="post" action="<?php echo esc_url( admin_url( 'admin-post.php' ) ); ?>">
					<input type="hidden" name="action" value="k54nl_toggle">
					<?php wp_nonce_field( 'k54nl_toggle' ); ?>
					<?php if ( 'active' === $st || 'pending' === $st ) : ?>
						<input type="hidden" name="on" value="0">
						<?php if ( 'pending' === $st ) : ?>
							<button type="submit" name="on" value="1">Bestätigungs-Mail erneut senden</button>
						<?php endif; ?>
						<button type="submit" class="k54nl-off" name="on" value="0">Update-Mails abbestellen</button>
					<?php else : ?>
						<button type="submit" name="on" value="1">Ja, bei Updates informieren</button>
					<?php endif; ?>
				</form>
				<p class="k54nl-small">Wir nutzen dafür nur deine Konto-E-Mail. Abmelden geht jederzeit hier oder mit einem Klick in jeder Mail. Mehr in der <a href="<?php echo esc_url( get_privacy_policy_url() ? get_privacy_policy_url() : home_url( '/datenschutz/' ) ); ?>" style="color:#d4af6a">Datenschutzerklärung</a>.</p>
			<?php endif; ?>
		</div>
		<?php
		return ob_get_clean();
	}

	public static function handle_toggle() {
		if ( ! is_user_logged_in() ) {
			wp_safe_redirect( self::member_url() );
			exit;
		}
		check_admin_referer( 'k54nl_toggle' );
		$uid = get_current_user_id();
		$on  = ! empty( $_POST['on'] ) && '1' === (string) wp_unslash( $_POST['on'] ); // phpcs:ignore WordPress.Security.ValidatedSanitizedInput
		if ( $on ) {
			if ( 'active' !== self::status( $uid ) ) {
				$last = (int) get_user_meta( $uid, self::M_ASKED, true );
				if ( time() - $last > 120 ) {           // nicht öfter als alle 2 Minuten
					update_user_meta( $uid, self::M_STATUS, 'pending' );
					update_user_meta( $uid, self::M_ASKED, time() );
					self::send_confirm( $uid );
				}
			}
			$code = 'active' === self::status( $uid ) ? 'ok' : 'sent';
		} else {
			self::unsubscribe( $uid );
			$code = 'off';
		}
		wp_safe_redirect( add_query_arg( 'k54nl', $code, self::member_url() ) . '#update-newsletter' );
		exit;
	}

	private static function send_confirm( $uid ) {
		$user = get_userdata( $uid );
		if ( ! $user || ! $user->user_email ) {
			return;
		}
		$site = wp_specialchars_decode( get_bloginfo( 'name' ), ENT_QUOTES );
		$body = 'Hallo ' . $user->display_name . ",\n\n" .
			"du möchtest per E-Mail erfahren, wenn es ein Update für die Keller54-Plugins gibt.\n" .
			"Bitte bestätige das mit einem Klick:\n\n" . self::link( 'k54nl_confirm', $uid ) . "\n\n" .
			"Wenn du das nicht warst, ignoriere diese Mail einfach – dann passiert nichts.\n\n" .
			"Viele Grüße\nFrank E. · Keller54";
		wp_mail( $user->user_email, '[' . $site . '] Bitte bestätige den Update-Newsletter', $body );
	}

	private static function unsubscribe( $uid ) {
		delete_user_meta( $uid, self::M_STATUS );
		delete_user_meta( $uid, self::M_CONSENT );
		delete_user_meta( $uid, self::M_ASKED );
		delete_user_meta( $uid, self::M_TOKEN );   // alte Links werden ungültig
	}

	/** Bestätigungs- und Abmelde-Links (funktionieren auch ohne Anmeldung). */
	public static function handle_links() {
		foreach ( array( 'k54nl_confirm', 'k54nl_unsub' ) as $action ) {
			if ( empty( $_GET[ $action ] ) || empty( $_GET['u'] ) ) { // phpcs:ignore WordPress.Security.NonceVerification
				continue;
			}
			$uid   = absint( $_GET['u'] );                                   // phpcs:ignore WordPress.Security.NonceVerification
			$given = sanitize_text_field( wp_unslash( $_GET[ $action ] ) ); // phpcs:ignore WordPress.Security.NonceVerification
			$have  = (string) get_user_meta( $uid, self::M_TOKEN, true );
			$code  = 'invalid';
			if ( $uid && strlen( $have ) >= 20 && hash_equals( $have, $given ) ) {
				if ( 'k54nl_confirm' === $action ) {
					update_user_meta( $uid, self::M_STATUS, 'active' );
					update_user_meta( $uid, self::M_CONSENT, current_time( 'mysql' ) );
					$code = 'ok';
				} else {
					self::unsubscribe( $uid );
					$code = 'off';
				}
			} elseif ( 'k54nl_unsub' === $action ) {
				$code = 'off';   // schon abgemeldet: freundlich bestätigen
			}
			wp_safe_redirect( add_query_arg( 'k54nl', $code, self::member_url() ) . '#update-newsletter' );
			exit;
		}
	}

	public static function forget_user( $uid ) {
		self::unsubscribe( $uid );
	}

	// ------------------------------------------------------------ Neue Version erkannt

	public static function products_added( $option, $value ) {
		self::products_changed( array(), $value );
	}

	public static function products_changed( $old, $new ) {
		$s = self::settings();
		if ( empty( $s['auto'] ) || ! is_array( $new ) ) {
			return;
		}
		$sent = get_option( self::OPT_SENT, array() );
		$sent = is_array( $sent ) ? $sent : array();
		foreach ( $new as $slug => $p ) {
			$v = isset( $p['version'] ) ? (string) $p['version'] : '';
			if ( '' === $v ) {
				continue;
			}
			if ( ! empty( $s['products'] ) && ! in_array( $slug, (array) $s['products'], true ) ) {
				continue;
			}
			$before = isset( $old[ $slug ]['version'] ) ? (string) $old[ $slug ]['version'] : '';
			if ( $v === $before || ( isset( $sent[ $slug ] ) && version_compare( $v, $sent[ $slug ], '<=' ) ) ) {
				continue;
			}
			$sent[ $slug ] = $v;
			update_option( self::OPT_SENT, $sent, false );
			// kurz warten, bis der Import fertig ist (alle Plattformen), dann in kleinen Paketen senden
			wp_schedule_single_event( time() + 120, 'k54nl_send', array( (string) $slug, $v, 0, 0 ) );
			self::log( 'Neue Version erkannt: ' . ( isset( $p['name'] ) ? $p['name'] : $slug ) . ' ' . $v . ' – Versand geplant.' );
		}
	}

	private static function subscribers( $offset, $limit ) {
		return get_users(
			array(
				'meta_key'   => self::M_STATUS, // phpcs:ignore WordPress.DB.SlowDBQuery
				'meta_value' => 'active',       // phpcs:ignore WordPress.DB.SlowDBQuery
				'orderby'    => 'ID',
				'order'      => 'ASC',
				'number'     => $limit,
				'offset'     => $offset,
			)
		);
	}

	private static function product( $slug ) {
		if ( class_exists( 'K54U_Storage' ) && method_exists( 'K54U_Storage', 'product' ) ) {
			$p = K54U_Storage::product( $slug );
			if ( is_array( $p ) ) {
				return $p;
			}
		}
		$all = get_option( 'k54u_products', array() );
		return isset( $all[ $slug ] ) && is_array( $all[ $slug ] ) ? $all[ $slug ] : array( 'name' => $slug );
	}

	private static function mail_text( $user, $slug, $version ) {
		$p     = self::product( $slug );
		$name  = isset( $p['name'] ) ? $p['name'] : strtoupper( $slug );
		$beta  = false !== stripos( $name, 'beta' );
		$notes = isset( $p['notes'] ) ? trim( wp_strip_all_tags( (string) $p['notes'] ) ) : '';
		if ( strlen( $notes ) > 1500 ) {
			$notes = rtrim( substr( $notes, 0, 1500 ) ) . " …\n(alle Neuerungen auf keller54.de)";
		}
		$page = ! empty( $p['page'] ) ? (string) $p['page'] : home_url( '/' . ( 'k54' === $slug ? 'k54' : $slug ) . '/' );
		$body = 'Hallo ' . $user->display_name . ",\n\n" .
			'es gibt eine neue Version: ' . $name . ' ' . $version . ".\n\n" .
			( $notes ? "Was ist neu:\n" . $notes . "\n\n" : '' ) .
			"So kommst du dran:\n" .
			'• Im Plugin auf UPDATE klicken – deine Einstellungen, Presets und Samples bleiben erhalten.' . "\n" .
			'• Oder im Mitgliederbereich unter Downloads: ' . self::member_url() . "\n" .
			'• Mehr dazu: ' . $page . "\n\n" .
			( $beta ? "Hinweis: Das ist eine Beta-Version unter Vorbehalt. Funktionen und Klänge können sich noch ändern, und es kann Fehler geben – bitte sichere deine Projekte vorher.\n\n" : '' ) .
			"Viele Grüße\nFrank E. · Keller54\n\n" .
			"—\nDu bekommst diese Mail, weil du den Update-Newsletter auf keller54.de bestätigt hast.\n" .
			'Abmelden mit einem Klick: ' . self::link( 'k54nl_unsub', $user->ID );
		return array( 'Neue Version: ' . $name . ' ' . $version, $body );
	}

	/** Ein Paket Mails senden; plant sich selbst für das nächste Paket neu ein. */
	public static function send_batch( $slug, $version, $offset = 0, $sent_count = 0, $only_user = 0 ) {
		$site  = wp_specialchars_decode( get_bloginfo( 'name' ), ENT_QUOTES );
		$users = $only_user ? array( get_userdata( $only_user ) ) : self::subscribers( (int) $offset, self::BATCH );
		$n     = 0;
		foreach ( $users as $u ) {
			if ( ! $u || ! $u->user_email ) {
				continue;
			}
			list( $subject, $body ) = self::mail_text( $u, $slug, $version );
			$headers = array( 'List-Unsubscribe: <' . self::link( 'k54nl_unsub', $u->ID ) . '>' );
			if ( wp_mail( $u->user_email, '[' . $site . '] ' . $subject, $body, $headers ) ) {
				++$n;
			}
		}
		if ( $only_user ) {
			return $n;
		}
		$total = (int) $sent_count + $n;
		if ( count( $users ) >= self::BATCH ) {
			wp_schedule_single_event( time() + 60, 'k54nl_send', array( (string) $slug, (string) $version, (int) $offset + self::BATCH, $total ) );
		} else {
			self::log( 'Versand ' . $slug . ' ' . $version . ' fertig: ' . $total . ' Mail(s).' );
		}
		return $n;
	}

	// ------------------------------------------------------------ Admin

	public static function menu() {
		add_options_page( 'Keller54 Update-Newsletter', 'Keller54 Newsletter', 'manage_options', 'k54nl', array( __CLASS__, 'page' ) );
	}

	private static function counts() {
		$c = array( 'active' => 0, 'pending' => 0 );
		foreach ( array_keys( $c ) as $st ) {
			$q = new WP_User_Query(
				array(
					'meta_key'    => self::M_STATUS, // phpcs:ignore WordPress.DB.SlowDBQuery
					'meta_value'  => $st,            // phpcs:ignore WordPress.DB.SlowDBQuery
					'count_total' => true,
					'fields'      => 'ID',
					'number'      => 1,
				)
			);
			$c[ $st ] = (int) $q->get_total();
		}
		return $c;
	}

	public static function page() {
		if ( ! current_user_can( 'manage_options' ) ) {
			return;
		}
		$s    = self::settings();
		$c    = self::counts();
		$all  = get_option( 'k54u_products', array() );
		$all  = is_array( $all ) ? $all : array();
		$log  = get_option( self::OPT_LOG, array() );
		?>
		<div class="wrap">
			<h1>Keller54 Update-Newsletter</h1>
			<p><strong><?php echo (int) $c['active']; ?></strong> bestätigte Empfänger · <?php echo (int) $c['pending']; ?> noch nicht bestätigt.</p>
			<p>Mitglieder tragen sich im Mitgliederbereich unten im Kasten „Update-Newsletter“ ein (oder überall mit dem Shortcode <code>[keller54_newsletter]</code>) und bestätigen per E-Mail-Link (Double-Opt-in). Jede Mail hat einen Abmelde-Link.</p>
			<form method="post" action="<?php echo esc_url( admin_url( 'admin-post.php' ) ); ?>">
				<input type="hidden" name="action" value="k54nl_save">
				<?php wp_nonce_field( 'k54nl_save' ); ?>
				<table class="form-table">
					<tr><th>Automatisch senden</th><td><label><input type="checkbox" name="auto" value="1" <?php checked( ! empty( $s['auto'] ) ); ?>> Bei jeder neuen Version im Download-Bereich eine Mail an alle bestätigten Empfänger schicken</label></td></tr>
					<tr><th>Produkte</th><td>
						<?php foreach ( $all as $slug => $p ) : ?>
							<label style="display:block"><input type="checkbox" name="products[]" value="<?php echo esc_attr( $slug ); ?>" <?php checked( empty( $s['products'] ) || in_array( $slug, (array) $s['products'], true ) ); ?>> <?php echo esc_html( ( isset( $p['name'] ) ? $p['name'] : $slug ) . ( ! empty( $p['version'] ) ? ' (' . $p['version'] . ')' : '' ) ); ?></label>
						<?php endforeach; ?>
						<p class="description">Nichts angehakt = alle Produkte.</p>
					</td></tr>
				</table>
				<?php submit_button( 'Speichern' ); ?>
			</form>
			<h2>Von Hand senden</h2>
			<form method="post" action="<?php echo esc_url( admin_url( 'admin-post.php' ) ); ?>">
				<input type="hidden" name="action" value="k54nl_manual">
				<?php wp_nonce_field( 'k54nl_manual' ); ?>
				<select name="product">
					<?php foreach ( $all as $slug => $p ) : ?>
						<option value="<?php echo esc_attr( $slug ); ?>"><?php echo esc_html( ( isset( $p['name'] ) ? $p['name'] : $slug ) . ' ' . ( isset( $p['version'] ) ? $p['version'] : '' ) ); ?></option>
					<?php endforeach; ?>
				</select>
				<button class="button" name="mode" value="test">Test-Mail an mich</button>
				<button class="button button-primary" name="mode" value="all" onclick="return confirm('Wirklich an alle bestätigten Empfänger senden?');">An alle Empfänger senden</button>
			</form>
			<h2>Protokoll</h2>
			<ul><?php foreach ( (array) $log as $l ) : ?><li><?php echo esc_html( $l ); ?></li><?php endforeach; ?></ul>
		</div>
		<?php
	}

	public static function save_settings() {
		if ( ! current_user_can( 'manage_options' ) ) {
			wp_die( 'Keine Berechtigung' );
		}
		check_admin_referer( 'k54nl_save' );
		$all      = get_option( 'k54u_products', array() );
		$products = isset( $_POST['products'] ) ? array_map( 'sanitize_title', (array) wp_unslash( $_POST['products'] ) ) : array();
		if ( is_array( $all ) && count( $products ) === count( $all ) ) {
			$products = array();   // alles angehakt = alle (auch künftige Produkte)
		}
		update_option(
			self::OPT,
			array(
				'auto'     => empty( $_POST['auto'] ) ? 0 : 1,
				'products' => $products,
			),
			false
		);
		wp_safe_redirect( admin_url( 'options-general.php?page=k54nl&saved=1' ) );
		exit;
	}

	public static function manual_send() {
		if ( ! current_user_can( 'manage_options' ) ) {
			wp_die( 'Keine Berechtigung' );
		}
		check_admin_referer( 'k54nl_manual' );
		$slug = isset( $_POST['product'] ) ? sanitize_title( wp_unslash( $_POST['product'] ) ) : '';
		$mode = isset( $_POST['mode'] ) ? sanitize_key( wp_unslash( $_POST['mode'] ) ) : 'test';
		$p    = self::product( $slug );
		$v    = isset( $p['version'] ) ? (string) $p['version'] : '';
		if ( 'all' === $mode ) {
			wp_schedule_single_event( time() + 5, 'k54nl_send', array( $slug, $v, 0, 0 ) );
			self::log( 'Versand von Hand gestartet: ' . $slug . ' ' . $v );
		} else {
			self::send_batch( $slug, $v, 0, 0, get_current_user_id() );
			self::log( 'Test-Mail an ' . wp_get_current_user()->user_email . ': ' . $slug . ' ' . $v );
		}
		wp_safe_redirect( admin_url( 'options-general.php?page=k54nl' ) );
		exit;
	}

	// ------------------------------------------------------------ REST (nur Admins)

	public static function routes() {
		$admin = function () {
			return current_user_can( 'manage_options' );
		};
		register_rest_route(
			'k54nl/v1',
			'/stats',
			array(
				'methods'             => 'GET',
				'permission_callback' => $admin,
				'callback'            => function () {
					return array(
						'counts'   => self::counts(),
						'settings' => self::settings(),
						'sent'     => get_option( self::OPT_SENT, array() ),
						'log'      => get_option( self::OPT_LOG, array() ),
					);
				},
			)
		);
		register_rest_route(
			'k54nl/v1',
			'/send',
			array(
				'methods'             => 'POST',
				'permission_callback' => $admin,
				'callback'            => function ( WP_REST_Request $req ) {
					$slug = sanitize_title( (string) $req->get_param( 'product' ) );
					$p    = self::product( $slug );
					$v    = isset( $p['version'] ) ? (string) $p['version'] : '';
					if ( '' === $v ) {
						return new WP_Error( 'k54nl', 'Unbekanntes Produkt oder keine Version.', array( 'status' => 400 ) );
					}
					if ( $req->get_param( 'test' ) ) {
						$n = self::send_batch( $slug, $v, 0, 0, get_current_user_id() );
						self::log( 'Test-Mail (REST): ' . $slug . ' ' . $v );
						return array( 'test' => true, 'sent' => $n );
					}
					wp_schedule_single_event( time() + 5, 'k54nl_send', array( $slug, $v, 0, 0 ) );
					self::log( 'Versand (REST) gestartet: ' . $slug . ' ' . $v );
					return array( 'scheduled' => true, 'product' => $slug, 'version' => $v, 'recipients' => self::counts()['active'] );
				},
			)
		);
	}
}

K54NL::init();

/**
 * Bei der Aktivierung: aktuelle Versionen als „schon angekündigt“ merken,
 * damit nicht sofort eine Mail für eine alte Version rausgeht.
 */
register_activation_hook(
	__FILE__,
	function () {
		$all  = get_option( 'k54u_products', array() );
		$sent = array();
		foreach ( (array) $all as $slug => $p ) {
			if ( ! empty( $p['version'] ) ) {
				$sent[ $slug ] = (string) $p['version'];
			}
		}
		update_option( K54NL::OPT_SENT, $sent, false );
	}
);
